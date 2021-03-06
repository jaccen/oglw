#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include "gl/gl.h"
#include "gl/vertexLayout.h"
#include "uniform.h"

namespace OGLW {

class Shader {

public:
    Shader();
    Shader(std::string _programBundlePath);
    Shader(std::string _fragPath, std::string _vertPath, std::string _geomPath = "");
    ~Shader();

    // load a bundle shader program
    bool loadBundleSource(const std::string& _bundleSource);

    // get the GL shader program handle
    GLuint getProgram() const;
    // get the GL fragment shader handle
    GLuint getFragmentShader() const;
    // get the GL vertex shader handle
    GLuint getVertexShader() const;

    // sets the current program
    void use() const;
    // checks whether the shader program is in use
    bool isInUse() const;

    // retrieve the attribute location for a given attribute name
    GLint getAttribLocation(const std::string& _attribute);
    // binds a vertex layout to the shader
    void bindVertexLayout(const VertexLayout& _layout);
    // set uniforms for the given name
    void setUniform(const std::string& _name, int _x);
    void setUniform(const std::string& _name, float _x);
    void setUniform(const std::string& _name, float _x, float _y);
    void setUniform(const std::string& _name, float _x, float _y, float _z);
    void setUniform(const std::string& _name, float _x, float _y, float _z, float _w);
    void setUniform(const std::string& _name, const glm::vec2& _value);
    void setUniform(const std::string& _name, const glm::vec3& _value);
    void setUniform(const std::string& _name, const glm::vec4& _value);
    void setUniform(const std::string& _name, const glm::mat2& _value, bool transpose = false);
    void setUniform(const std::string& _name, const glm::mat3& _value, bool transpose = false);
    void setUniform(const std::string& _name, const glm::mat4& _value, bool transpose = false);

    static std::string stringFromKind(GLenum _kind);

private:
    bool linkShaderProgram(GLuint _program);

    // compile and attach a shader to the shader program
    bool add(const std::string& _shaderSource, GLenum _kind, GLuint& _shader);
    // retrive a shader source from a program bundle
    bool getBundleShaderSource(std::string _type, std::string _bundle, std::string* _out, bool _opt = false) const;
    // load a shader program
    bool load(const std::string& _fragmentSrc, const std::string& _vertexSrc, const std::string& _geomSrc);
    // compile the shader program for the specified type
    bool compile(const std::string& _src, GLenum _type, GLuint& _shader);
    // retrieve the uniform location for a given name, lazily access the driver to request for uniform location
    GLint getUniformLocation(const std::string& _uniformName);

    // GL shader program handle
    GLuint m_program = -1;
    // GL fragment shader id
    GLuint m_fragmentShader = -1;
    // GL vertex shader id
    GLuint m_vertexShader = -1;

    std::unordered_map<std::string, GLuint> m_uniforms;
    std::unordered_map<std::string, GLuint> m_attributes;

    // Map of uniform cache associated with a transpose flag
    std::unordered_map<GLint, std::pair<UniformValue, bool>> m_uniformCache;

    template <class T>
    inline bool getFromCache(GLint _location, T _value, bool _transpose = false) {
        using uniformPair = std::pair<UniformValue, bool>;
        const auto& v = m_uniformCache.find(_location);
        bool cached = false;
        if (v != m_uniformCache.end()) {
            if (v->second.first.is<T>()) {
                T& value = v->second.first.get<T>();
                cached = (value == _value) && (v->second.second == _transpose);
                if (!cached) {
                    value = _value;
                    v->second.second = _transpose;
                }
            }
        } else { m_uniformCache[_location] = uniformPair(_value, _transpose); }
        return cached;
    }
};

} // OGLW
