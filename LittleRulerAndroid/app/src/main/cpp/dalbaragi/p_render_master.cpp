#include "p_render_master.h"

#include <string>
#include <vector>
#include <memory>

#include <glm/gtc/matrix_transform.hpp>

#include "u_fileclass.h"
#include "s_logger_god.h"
#include "s_scripting.h"
#include "o_widget_texview.h"


using namespace std::string_literals;


namespace {

#ifdef _WIN32
    void glDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
        dalWarn(message);
    }
#endif

}


// Main Framebuffer
namespace dal {

    RenderMaster::MainFramebuffer::MainFramebuffer(const unsigned int widWidth, const unsigned int widHeight)
        : m_renderScale(1.0f)
        , m_bufWidth((unsigned int)(float(widWidth)* m_renderScale))
        , m_bufHeight((unsigned int)(float(widHeight)* m_renderScale))
    {
        // Establish framebuffer
        {
            glGenFramebuffers(1, &m_mainFbuf);
            glBindFramebuffer(GL_FRAMEBUFFER, this->m_mainFbuf);

            glGenTextures(1, &this->m_colorMap);
            glBindTexture(GL_TEXTURE_2D, this->m_colorMap);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->m_bufWidth, this->m_bufHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->m_colorMap, 0);
            glBindTexture(GL_TEXTURE_2D, 0);

            glGenRenderbuffers(1, &this->m_mainRenderbuf);
            glBindRenderbuffer(GL_RENDERBUFFER, this->m_mainRenderbuf);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, this->m_bufWidth, this->m_bufHeight);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, this->m_mainRenderbuf);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            if ( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE ) {
                dalAbort("Failed to create framebuffer.");
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // Establish vbo for fbuffer
        {
            glGenVertexArrays(1, &this->m_vbo);
            if ( this->m_vbo <= 0 ) dalAbort("Failed gen vertex array.");
            glGenBuffers(1, &this->m_vertexArr);
            if ( m_vertexArr <= 0 ) dalAbort("Failed to gen a vertex buffer.");
            glGenBuffers(1, &this->m_texcoordArr);
            if ( this->m_texcoordArr <= 0 ) dalAbort("Failed to gen a texture coordinate buffer.");

            glBindVertexArray(this->m_vbo);

            // Vertices
            {
                GLfloat vertices[12] = {
                    -1,  1,
                    -1, -1,
                     1, -1,
                    -1,  1,
                     1, -1,
                     1,  1
                };
                auto size = 12 * sizeof(float);

                glBindBuffer(GL_ARRAY_BUFFER, this->m_vertexArr);
                glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
            }

            // TexCoords
            {
                GLfloat texCoords[12] = {
                    0, 1,
                    0, 0,
                    1, 0,
                    0, 1,
                    1, 0,
                    1, 1
                };
                auto size = 12 * sizeof(float);

                glBindBuffer(GL_ARRAY_BUFFER, this->m_texcoordArr);
                glBufferData(GL_ARRAY_BUFFER, size, texCoords, GL_STATIC_DRAW);

                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
            }

            glBindVertexArray(0);
        }

        this->m_tex = new Texture(this->m_colorMap);
    }

    RenderMaster::MainFramebuffer::~MainFramebuffer(void) {
        glDeleteBuffers(1, &this->m_vertexArr);
        glDeleteBuffers(1, &this->m_texcoordArr);
        glDeleteVertexArrays(1, &this->m_vbo);

        glDeleteRenderbuffers(1, &this->m_mainRenderbuf);
        glDeleteTextures(1, &this->m_colorMap);
        glDeleteFramebuffers(1, &this->m_mainFbuf);

        glDeleteFramebuffers(1, &m_mainFbuf);
    }

    void RenderMaster::MainFramebuffer::setRenderScale(float v, unsigned int win_width, unsigned int win_height) {
        m_renderScale = v;
        auto w = static_cast<unsigned int>(float(win_width) * v);
        auto h = static_cast<unsigned int>(float(win_height) * v);
        this->resizeFbuffer(w, h);
    }

    void RenderMaster::MainFramebuffer::resizeFbuffer(unsigned int newWin_width, unsigned int newWin_height) {
        this->m_bufWidth = (unsigned int)(float(newWin_width) * this->m_renderScale);
        this->m_bufHeight = (unsigned int)(float(newWin_height) * this->m_renderScale);

        glBindTexture(GL_TEXTURE_2D, this->m_colorMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->m_bufWidth, this->m_bufHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindRenderbuffer(GL_RENDERBUFFER, this->m_mainRenderbuf);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, this->m_bufWidth, this->m_bufHeight);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    void RenderMaster::MainFramebuffer::startRenderOn(void) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_mainFbuf);
        glViewport(0, 0, m_bufWidth, m_bufHeight);
    }

    void RenderMaster::MainFramebuffer::renderOnScreen(const UnilocFScreen& uniloc) {
        glBindVertexArray(m_vbo);

        this->m_tex->sendUniform(uniloc.uTexture, 0, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    /*
    Texture* RenderMaster::MainFramebuffer::getTex(void) {
        return this->m_tex;
    }
    */

}


// Render Master
namespace dal {

    RenderMaster::RenderMaster(SceneMaster& scene, ShaderMaster& shader, OverlayMaster& overlay, ICamera* const camera, const unsigned int winWidth, const unsigned int winHeight)
        : m_scene(scene),
        m_shader(shader),
        m_fbuffer(winWidth, winHeight),
        m_winWidth(winWidth), m_winHeight(winHeight),
        m_projectMat(1.0),
        m_flagDrawDlight1(true),
        m_skyColor(0.6f, 0.6f, 0.9f),
        m_mainCamera(camera)
    {
        // Lights
        {
            m_dlight1.m_color = { 0.7, 0.7, 0.7 };
            m_dlight1.setDirectin(-1.8f, -1.0f, 2.0f);
        }

        // OpenGL global switch
        {
            glClearColor(m_skyColor.x, m_skyColor.y, m_skyColor.z, 1.0f);
#ifdef _WIN32
            glDebugMessageCallback(glDebugCallback, nullptr);
#endif
        }

        // View
        {
            {
                auto water = this->m_scene.getWater("test_level", 0);
                dalAssert(water != nullptr);
                auto view = new TextureView(nullptr, water->m_fbuffer.getReflectionTexture());
                view->setPosX(10.0f);
                view->setPosY(30.0f);
                view->setWidth(128.0f);
                view->setHeight(128.0f);
                view->setPauseOnly(false);
                overlay.addWidget(view);
            }

            {
                auto tex = this->m_dlight1.getShadowMap();
                auto view = new TextureView(nullptr, tex);
                view->setPosX(10.0f);
                view->setPosY(30.0f + 130.0f);
                view->setWidth(128.0f);
                view->setHeight(128.0f);
                view->setPauseOnly(false);
                overlay.addWidget(view);
            }
        }

        // Misc
        {
            float radio = static_cast<float>(m_winWidth) / static_cast<float>(m_winHeight);
            this->m_projectMat = glm::perspective(glm::radians(90.0f), radio, 0.01f, 100.0f);

            script::init_renderMas(this);
        }
    }

    void RenderMaster::update(const float deltaTime) {
        this->m_scene.update(deltaTime);

        /*
        const auto mat = glm::rotate(glm::mat4{ 1.0f }, deltaTime * 0.3f, glm::vec3{ 1.0f, 0.5f, 0.0f });
        const glm::vec4 direcBefore{ this->m_dlight1.getDirection(), 0.0f };
        const auto newDirec = glm::normalize(glm::vec3{ mat * direcBefore });
        this->m_dlight1.setDirectin(newDirec);

        const auto diff = glm::dot(newDirec, glm::vec3{ 0.0f, -1.0f, 0.0f });
        m_skyColor = glm::vec3{ 0.5f, 0.5f, 0.8f } *glm::max(diff, 0.0f) + glm::vec3{ 0.1f, 0.1f, 0.2f };
        glClearColor(m_skyColor.x, m_skyColor.y, m_skyColor.z, 1.0f);

        this->m_flagDrawDlight1 = -0.3f < diff;
        */
    }

    void RenderMaster::render(void) {
        // Shadow map
        {
            this->m_dlight1.clearDepthBuffer();

            {
                auto& uniloc = this->m_shader.useDepthMp();
                m_dlight1.startRenderShadowmap(uniloc.m_geometry);
                m_scene.renderDepthMp(uniloc);
            }

            {
                auto& uniloc = this->m_shader.useDepthAnime();
                this->m_dlight1.startRenderShadowmap(uniloc.m_geometry);
                this->m_scene.renderDepthAnimated(uniloc);
            }

            m_dlight1.finishRenderShadowmap();
        }

        // Render to water framebuffer
        {
#ifdef _WIN32
            glEnable(GL_CLIP_DISTANCE0);
#endif
            auto& unilocGeneral = this->m_shader.useGeneral();

            unilocGeneral.m_planeClip.flagDoClip(true);

            unilocGeneral.m_lightedMesh.projectMat(this->m_projectMat);

            unilocGeneral.m_lightedMesh.baseAmbient(0.3f, 0.3f, 0.3f);

            // Lights

            if ( this->m_flagDrawDlight1 ) {
                this->m_dlight1.sendUniform(unilocGeneral.m_lightedMesh, 0);
                unilocGeneral.m_lightedMesh.dlightCount(1);
            }
            else {
                unilocGeneral.m_lightedMesh.dlightCount(0);
            }

            // Render meshes

            this->m_scene.renderGeneral_onWater(unilocGeneral, *this->m_mainCamera);
        }

        // Render animated to water framebuffer
        {
#ifdef _WIN32
            glEnable(GL_CLIP_DISTANCE0);
#endif
            auto& unilocGeneral = this->m_shader.useAnimate();

            unilocGeneral.m_planeClip.flagDoClip(true);

            unilocGeneral.m_lightedMesh.projectMat(this->m_projectMat);

            unilocGeneral.m_lightedMesh.baseAmbient(0.3f, 0.3f, 0.3f);

            // Lights

            if ( this->m_flagDrawDlight1 ) {
                this->m_dlight1.sendUniform(unilocGeneral.m_lightedMesh, 0);
                unilocGeneral.m_lightedMesh.dlightCount(1);
            }
            else {
                unilocGeneral.m_lightedMesh.dlightCount(0);
            }

            // Render meshes

            this->m_scene.renderAnimate_onWater(unilocGeneral, *this->m_mainCamera);
        }

        this->m_fbuffer.startRenderOn();
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
#ifdef _WIN32
        glDisable(GL_CLIP_DISTANCE0);
#endif

        // Render to framebuffer 
        {
            auto& unilocGeneral = this->m_shader.useGeneral();

            unilocGeneral.m_lightedMesh.projectMat(this->m_projectMat);
            unilocGeneral.m_planeClip.flagDoClip(false);
            unilocGeneral.m_lightedMesh.viewMat(this->m_mainCamera->getViewMat());
            unilocGeneral.m_lightedMesh.viewPos(this->m_mainCamera->m_pos);

            // Render meshes

            this->m_scene.renderGeneral(unilocGeneral);
        }

        // Render to framebuffer animated
        {
            auto& unilocGeneral = this->m_shader.useAnimate();

            unilocGeneral.m_lightedMesh.projectMat(this->m_projectMat);
            unilocGeneral.m_planeClip.flagDoClip(false);
            unilocGeneral.m_lightedMesh.viewMat(this->m_mainCamera->getViewMat());
            unilocGeneral.m_lightedMesh.viewPos(this->m_mainCamera->m_pos);

            // Render meshes

            this->m_scene.renderAnimate(unilocGeneral);
        }

        // Render water to framebuffer
        {
            auto& unilocWaterry = this->m_shader.useWaterry();

            unilocWaterry.m_lightedMesh.projectMat(this->m_projectMat);
            unilocWaterry.m_lightedMesh.viewMat(this->m_mainCamera->getViewMat());
            unilocWaterry.m_lightedMesh.viewPos(this->m_mainCamera->m_pos);
            unilocWaterry.m_lightedMesh.baseAmbient(0.3f, 0.3f, 0.3f);

            // Lights

            if ( this->m_flagDrawDlight1 ) {
                this->m_dlight1.sendUniform(unilocWaterry.m_lightedMesh, 0);
                unilocWaterry.m_lightedMesh.dlightCount(1);
            }
            else {
                unilocWaterry.m_lightedMesh.dlightCount(0);
            }

            // Render meshes

            this->m_scene.renderWaterry(unilocWaterry);
        }

        // Render framebuffer to quad 
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, this->m_winWidth, this->m_winHeight);

            auto& uniloc = this->m_shader.useFScreen();
            this->m_fbuffer.renderOnScreen(uniloc);
        }
    }

    void RenderMaster::setRenderScale(float v) {
        this->m_fbuffer.setRenderScale(v, m_winWidth, m_winHeight);
    }

    void RenderMaster::onWinResize(const unsigned int width, const unsigned int height) {
        this->m_winWidth = width;
        this->m_winHeight = height;

        float radio = static_cast<float>(width) / static_cast<float>(height);
        this->m_projectMat = glm::perspective(glm::radians(90.0f), radio, 0.01f, 100.0f);

        this->m_fbuffer.resizeFbuffer(width, height);
    }

    ICamera* RenderMaster::replaceMainCamera(ICamera* camera) {
        auto tmp = this->m_mainCamera;
        this->m_mainCamera = camera;
        return tmp;
    }

}