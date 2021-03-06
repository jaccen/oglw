#include "shader.h"
#include "core/utils.h"
#include "core/log.h"
#include "gl/renderState.h"

namespace OGLW {

Shader::Shader(std::string _fragPath, std::string _vertPath, std::string _geomPath) {
    std::string vert, frag, geom;
    stringFromPath(_vertPath, &vert);
    stringFromPath(_fragPath, &frag);

    if (_geomPath != "") {
        stringFromPath(_geomPath, &geom);
    }

    if (!load(frag, vert, geom)) {
        WARN("Failed to build shader %s %s\n", _fragPath.c_str(), _vertPath.c_str());
    }
}

Shader::Shader() {}

Shader::Shader(std::string _programBundlePath) {
    std::string bundle;
    stringFromPath(_programBundlePath, &bundle);

    if (!loadBundleSource(bundle)) {
        WARN("Failed to build shader program bundle %s\n", _programBundlePath.c_str());
    }
}

bool Shader::loadBundleSource(const std::string& _bundleSource) {
    std::string vert, frag, geom;

    if (getBundleShaderSource("vertex", _bundleSource, &vert) &&
        getBundleShaderSource("fragment", _bundleSource, &frag)) {

        // geometry shader is optionnal
        getBundleShaderSource("geom", _bundleSource, &geom, true);

        if (!load(frag, vert, geom)) {
            return false;
        }
    } else {
        return false;
    }

    return true;
}

Shader::~Shader() {
    GL_CHECK(glDeleteProgram(m_program));

    // Deleting a shader program being used ends up setting up the current shader program to 0
    // after the driver finishes using it, force this setup by setting the current program
    if (RenderState::shaderProgram.compare(m_program)) {
        RenderState::shaderProgram.init(0, false);
    }
}

bool Shader::getBundleShaderSource(std::string _type, std::string _bundle, std::string* _out, bool _opt) const {
    const std::string startTag = "#pragma begin:" + _type;
    const std::string endTag = "#pragma end:" + _type;

    size_t start = _bundle.find(startTag);
    start += startTag.length();

    if (start == std::string::npos) {
        if (!_opt) {
            WARN("Missing tag %s in shader bundle\n", startTag.c_str());
        }
        return false;
    }

    size_t end = _bundle.find(endTag);

    if ((end < start || end == std::string::npos)) {
        if (!_opt) {
            WARN("Missing tag %s in shader bundle\n", endTag.c_str());
        }
        return false;
    } else if (end > start) {
        *_out = _bundle.substr(start, end - start);
        return true;
    }

    return false;
}

bool Shader::add(const std::string& _shaderSource, GLenum _kind, GLuint& _shader) {
    if (!compile(_shaderSource, _kind, _shader)) {
        WARN("Failed to compile shader of type %s\n", Shader::stringFromKind(_kind).c_str());
        WARN("%s\n", _shaderSource.c_str());
        return false;
    }

    GL_CHECK(glAttachShader(m_program, _shader));

    return true;
}

bool Shader::load(const std::string& _fragmentSrc, const std::string& _vertexSrc, const std::string& _geomSrc) {
    bool addShaders = true;
    GLuint vert = 0, frag = 0, geom = 0;

    m_program = glCreateProgram();
    GL_CHECK(void(0));

    // add vertex shader to shader program
    addShaders |= add(_vertexSrc, GL_VERTEX_SHADER, vert);

    // add fragment shader to shader program
    addShaders |=  add(_fragmentSrc, GL_FRAGMENT_SHADER, frag);

    if (!_geomSrc.empty()) {
        // add geometry shader to shader program
        addShaders |= add(_geomSrc, GL_GEOMETRY_SHADER, geom);
    }

    if (!addShaders) {
        WARN("Delete shader program\n");
        GL_CHECK(glDeleteProgram(m_program));
        return false;
    }

    bool linkStatus = linkShaderProgram(m_program);

    GL_CHECK(glDeleteShader(vert));
    GL_CHECK(glDeleteShader(frag));

    if (!_geomSrc.empty()) {
        GL_CHECK(glDeleteShader(geom));
    }

    if (!linkStatus) {
        GL_CHECK(glDeleteProgram(m_program));
        return false;
    }

    return true;
}

bool Shader::linkShaderProgram(GLuint _program) {
    GLint isLinked;

    m_uniformCache.clear();

    GL_CHECK(glLinkProgram(_program));
    GL_CHECK(glGetProgramiv(_program, GL_LINK_STATUS, &isLinked));

    if (isLinked == GL_FALSE) {
        GLint infoLength = 0;
        GL_CHECK(glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &infoLength));

        if (infoLength > 1) {
            std::vector<GLchar> infoLog(infoLength);
            GL_CHECK(glGetProgramInfoLog(_program, infoLength, NULL, &infoLog[0]));
            std::string error(infoLog.begin(), infoLog.end());
            DBG("Error linking shader program\n");
            DBG("%s", error.c_str());
        }

        return false;
    }

    return true;
}

void Shader::bindVertexLayout(const VertexLayout& _layout) {
    auto locations = _layout.getLocations();
    bool needLink = false;

    for (const auto& loc : locations) {
        auto attributePair = m_attributes.find(loc.first);

        if (attributePair == m_attributes.end() || attributePair->second != loc.second) {
            WARN("Binding location %d for attribute %s\n", loc.second, loc.first.c_str());
            GL_CHECK(glBindAttribLocation(m_program, loc.second, loc.first.c_str()));
            m_attributes[loc.first] = loc.second;
            needLink = true;
        }
    }

    if (needLink) {
        WARN("Relink shader\n");
        linkShaderProgram(m_program);
    }
}

GLint Shader::getAttribLocation(const std::string& _attribute) {
    auto attributePair = m_attributes.find(_attribute);

    if (attributePair == m_attributes.end()) {
        GLint attribute = glGetAttribLocation(m_program, _attribute.c_str());
        GL_CHECK(void(0));

        if (attribute == -1) {
            WARN("Attribute location not found on shader for attribute %s\n", _attribute.c_str());
        } else {
            m_attributes[_attribute] = attribute;
        }
        return attribute;
    }

    return attributePair->second;
}

void Shader::use() const {
    RenderState::shaderProgram(m_program);
}

bool Shader::compile(const std::string& _src, GLenum _type, GLuint& _shader) {
    _shader = glCreateShader(_type);
    const GLchar* source = (const GLchar*)_src.c_str();

    GL_CHECK(glShaderSource(_shader, 1, &source, NULL));
    GL_CHECK(glCompileShader(_shader));

    GLint isCompiled;
    GL_CHECK(glGetShaderiv(_shader, GL_COMPILE_STATUS, &isCompiled));

    if (isCompiled == GL_FALSE) {
        GLint infoLength = 0;
        GL_CHECK(glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &infoLength));
        if (infoLength > 1) {
            std::vector<GLchar> infoLog(infoLength);
            GL_CHECK(glGetShaderInfoLog(_shader, infoLength, NULL, &infoLog[0]));
            DBG("Compilation error\n");
            DBG("%s", &infoLog[0]);
        }
        GL_CHECK(glDeleteShader(_shader));
        return false;
    }

    return true;
}

GLint Shader::getUniformLocation(const std::string& _uniformName) {
    auto uniformPair = m_uniforms.find(_uniformName);

    if (uniformPair == m_uniforms.end()) {
        GLint loc = glGetUniformLocation(m_program, _uniformName.c_str());
        GL_CHECK(void(0));

        if (loc == -1) {
            // not to overflow log, notify once
            WARN("Shader uniform %s not found on shader program: %d\n", _uniformName.c_str(), m_program);
        } else {
            m_uniforms[_uniformName] = loc;
        }
        return loc;
    }

    // directly access the uniform from the map
    return uniformPair->second;
}


void Shader::setUniform(const std::string& _name, int _x) {
    use();
    GLint location = getUniformLocation(_name);
    if (location >= 0) {
        bool cached = getFromCache(location, _x);
        if (!cached) {
            GL_CHECK(glUniform1i(location, _x));
        }
    }
}


void Shader::setUniform(const std::string& _name, float _x) {
    use();
    GLint location = getUniformLocation(_name);
    if (location >= 0) {
        bool cached = getFromCache(location, _x);
        if (!cached) {
            GL_CHECK(glUniform1f(location, _x));
        }
    }
}

void Shader::setUniform(const std::string& _name, float _x, float _y) {
    use();
    GLint location = getUniformLocation(_name);
    if (location >= 0) {
        bool cached = getFromCache(location, glm::vec2(_x, _y));
        if (!cached) {
            GL_CHECK(glUniform2f(location, _x, _y));
        }
    }
}

void Shader::setUniform(const std::string& _name, float _x, float _y, float _z) {
    use();
    GLint location = getUniformLocation(_name);
    if (location >= 0) {
        bool cached = getFromCache(location, glm::vec3(_x, _y, _z));
        if (!cached) {
            GL_CHECK(glUniform3f(location, _x, _y, _z));
        }
    }
}

void Shader::setUniform(const std::string& _name, float _x, float _y, float _z, float _w) {
    use();
    GLint location = getUniformLocation(_name);
    if (location >= 0) {
        bool cached = getFromCache(location, glm::vec4(_x, _y, _z, _w));
        if (!cached) {
            GL_CHECK(glUniform4f(location, _x, _y, _z, _w));
        }
    }
}

void Shader::setUniform(const std::string& _name, const glm::mat2& _value, bool _transpose) {
    use();
    GLint location = getUniformLocation(_name);
    if (location >= 0) {
        bool cached = getFromCache(location, _value, _transpose);
        if (!cached) {
            GL_CHECK(glUniformMatrix2fv(location, 1, _transpose, &_value[0][0]));
        }
    }
}

void Shader::setUniform(const std::string& _name, const glm::mat3& _value, bool _transpose) {
    use();
    GLint location = getUniformLocation(_name);
    if (location >= 0) {
        bool cached = getFromCache(location, _value, _transpose);
        if (!cached) {
            GL_CHECK(glUniformMatrix3fv(location, 1, _transpose, &_value[0][0]));
        }
    }
}

void Shader::setUniform(const std::string& _name, const glm::mat4& _value, bool _transpose) {
    use();
    GLint location = getUniformLocation(_name);
    if (location >= 0) {
        bool cached = getFromCache(location, _value, _transpose);
        if (!cached) {
            GL_CHECK(glUniformMatrix4fv(location, 1, _transpose, &_value[0][0]));
        }
    }
}

void Shader::setUniform(const std::string& _name, const glm::vec2& _value) {
    setUniform(_name, _value.x, _value.y);
}

void Shader::setUniform(const std::string& _name, const glm::vec3& _value) {
    setUniform(_name, _value.x, _value.y, _value.z);
}

void Shader::setUniform(const std::string& _name, const glm::vec4& _value) {
    setUniform(_name, _value.x, _value.y, _value.z, _value.w);
}

GLuint Shader::getProgram() const {
    return m_program;
}

GLuint Shader::getFragmentShader() const {
    return m_fragmentShader;
}

GLuint Shader::getVertexShader() const {
    return m_vertexShader;
}

std::string Shader::stringFromKind(GLenum _kind) {
    switch(_kind) {
        case GL_GEOMETRY_SHADER: return "geometry shader";
        case GL_FRAGMENT_SHADER: return "fragment shader";
        case GL_VERTEX_SHADER: return "vertex shader";
        default: return "unknown shader type";
    }
}

} // OGLW
