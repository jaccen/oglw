#include <vector>
#include <string>
#include <memory>
#include "oglw.h"

template <class T>
using uptr = std::unique_ptr<T>;
using namespace OGLW;

// ------------------------------------------------------------------------------
// OGLW App
class TestApp : public App {
    public:
        TestApp() : App("OGLW::TestApp", 800, 600) {}
        void update(float _dt) override;
        void render(float _dt) override;
        void init() override;

    private:
        uptr<Shader> m_shader;

        uptr<Mesh<glm::vec4>> m_geometry;
        uptr<Texture> m_texture;
        float m_xrot = 0.f, m_yrot = 0.f;
};
OGLWMain(TestApp);

void TestApp::init() {
    m_camera.setPosition({0.0, -0.5, 14.0});

    m_shader = uptr<Shader>(new Shader("default.frag", "default.vert"));
    OGLW::TextureOptions options = {
        GL_RGBA8, GL_RGBA, 
        {GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR}, 
        {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}
    };
    m_texture = uptr<OGLW::Texture>(new OGLW::Texture("merc_hillshade_av.png", options, true));

    m_geometry = plane(2.5f, 2.5f, 100, 100);
}

void TestApp::update(float _dt) {
    m_xrot += m_cursorX;
    m_yrot += m_cursorY;
}

void TestApp::render(float _dt) {
    glm::mat4 model;
    glm::mat4 view = m_camera.getViewMatrix();

    model = glm::rotate(model, m_xrot * 1e-2f, glm::vec3(0.0, 1.0, 0.0));
    model = glm::rotate(model, m_yrot * 1e-2f, glm::vec3(1.0, 0.0, 0.0));

    m_texture->bind(0);

    glm::mat4 mvp = m_camera.getProjectionMatrix() * view * model;

    m_shader->setUniform("mvp", mvp);
    m_shader->setUniform("tex", 0);


    m_geometry->draw(*m_shader);
}

