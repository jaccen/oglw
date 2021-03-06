#include "textureCube.h"
#include "core/utils.h"
#include "core/log.h"

namespace OGLW {

TextureCube::TextureCube(std::string _file, TextureOptions _options) : Texture(0, 0, _options) {

    m_target = GL_TEXTURE_CUBE_MAP;
    load(_file);
}

void TextureCube::load(const std::string& _file) {
    uint size;
    uchar* data = bytesFromPath(_file.c_str(), &size);
    uchar* pixels;
    int width, height, comp;

    pixels = stbi_load_from_memory(data, size, &width, &height, &comp, STBI_rgb_alpha);

    if (!pixels || size == 0) {
        WARN("Failed to load texture cube resource %s\n", _file.c_str());
        free(data);
        return;
    }

    size = width * height;

    m_width = width / 4;
    m_height = height / 3;

    for (int i = 0; i < 6; ++i) {
        GLenum face = CubeMapFace[i];
        m_faces.push_back({face, {}, 0});
        m_faces.back().m_data.resize(m_width * m_height);
    }

    for (int y = 0; y < height; ++y) {
        int jFace = (y - (y % m_height)) / m_height;

        for (int iFace = 0; iFace < 4; ++iFace) {
            Face* face = nullptr;

            if (iFace == 2 && jFace == 1)
                face = &m_faces[0]; // +x
            if (iFace == 0 && jFace == 1)
                face = &m_faces[1]; // -x
            if (iFace == 1 && jFace == 0)
                face = &m_faces[2]; // +y
            if (iFace == 1 && jFace == 2)
                face = &m_faces[3]; // -y
            if (iFace == 1 && jFace == 1)
                face = &m_faces[4]; // +z
            if (iFace == 3 && jFace == 1)
                face = &m_faces[5]; // -z

            if (!face) {
                continue;
            }

            int offset = (m_width * iFace + y * width) * sizeof(uint);
            std::memcpy(face->m_data.data() + face->m_offset, pixels + offset, m_width * sizeof(uint));
            face->m_offset += m_width;
        }
    }

    free(data);
    stbi_image_free(pixels);

    update(0);
}

void TextureCube::update(GLuint _textureUnit) {
    if (m_glHandle != 0 || m_faces.size() == 0) {
        return;
    }

    generate(_textureUnit);

    for (int i = 0; i < 6; ++i) {
        Face& f = m_faces[i];
        GL_CHECK(glTexImage2D(CubeMapFace[i], 0, m_options.internalFormat, m_width, m_height, 0, m_options.format,
                     m_options.type, f.m_data.data()));
        f.m_data.clear();
    }
}

} // OGLW
