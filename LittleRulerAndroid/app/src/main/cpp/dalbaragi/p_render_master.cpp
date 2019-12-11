#include "p_render_master.h"

#include <string>
#include <vector>
#include <memory>

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>

#include "s_logger_god.h"


using namespace fmt::literals;


namespace {

	constexpr unsigned int MAX_SCREEN_RES = 720;

#ifdef _WIN32
    void GLAPIENTRY glDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
        dalWarn(message);
    }
#endif

}


namespace {

    class SkyboxRenderer {

    private:
        GLuint m_vao = 0, m_vbo = 0;

    public:
        void init(void) {
            const float skyboxVertices[] = {
                // positions
                -1.0f,  1.0f, -1.0f,
                -1.0f, -1.0f, -1.0f,
                 1.0f, -1.0f, -1.0f,
                 1.0f, -1.0f, -1.0f,
                 1.0f,  1.0f, -1.0f,
                -1.0f,  1.0f, -1.0f,

                -1.0f, -1.0f,  1.0f,
                -1.0f, -1.0f, -1.0f,
                -1.0f,  1.0f, -1.0f,
                -1.0f,  1.0f, -1.0f,
                -1.0f,  1.0f,  1.0f,
                -1.0f, -1.0f,  1.0f,

                 1.0f, -1.0f, -1.0f,
                 1.0f, -1.0f,  1.0f,
                 1.0f,  1.0f,  1.0f,
                 1.0f,  1.0f,  1.0f,
                 1.0f,  1.0f, -1.0f,
                 1.0f, -1.0f, -1.0f,

                -1.0f, -1.0f,  1.0f,
                -1.0f,  1.0f,  1.0f,
                 1.0f,  1.0f,  1.0f,
                 1.0f,  1.0f,  1.0f,
                 1.0f, -1.0f,  1.0f,
                -1.0f, -1.0f,  1.0f,

                -1.0f,  1.0f, -1.0f,
                 1.0f,  1.0f, -1.0f,
                 1.0f,  1.0f,  1.0f,
                 1.0f,  1.0f,  1.0f,
                -1.0f,  1.0f,  1.0f,
                -1.0f,  1.0f, -1.0f,

                -1.0f, -1.0f, -1.0f,
                -1.0f, -1.0f,  1.0f,
                 1.0f, -1.0f, -1.0f,
                 1.0f, -1.0f, -1.0f,
                -1.0f, -1.0f,  1.0f,
                 1.0f, -1.0f,  1.0f
            };

            glGenVertexArrays(1, &this->m_vao);
            glGenBuffers(1, &this->m_vbo);
            glBindVertexArray(this->m_vao);
            glBindBuffer(GL_ARRAY_BUFFER, this->m_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        }

        void render(const dal::UnilocSkybox& uniloc, const dal::CubeMap& cubemap) const {
            glDepthMask(GL_FALSE);
            glDepthFunc(GL_LEQUAL);

            glBindVertexArray(this->m_vao);
            cubemap.sendUniform(uniloc.getSkyboxTexLoc());
            glDrawArrays(GL_TRIANGLES, 0, 36);

            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);
        }

    } g_skyRenderer;


    class CubeMapFbuf {

    private:
        GLuint m_fbo = 0;
        std::shared_ptr<dal::CubeMap> m_cubemap;
        inline static constexpr unsigned WIDTH = 256, HEIGHT = 256;

    public:
        void init(void) {
            glGenFramebuffers(1, &this->m_fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, this->m_fbo);

            this->m_cubemap.reset(new dal::CubeMap);
            this->m_cubemap->initAttach_colorMap(this->WIDTH, this->HEIGHT);
        }

        void bindFbuf(void) {
            glBindFramebuffer(GL_FRAMEBUFFER, this->m_fbo);
            glViewport(0, 0, this->WIDTH, this->HEIGHT);
        }

        void unbindFbuf(const unsigned width, const unsigned height) {
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->m_fbo);
            glViewport(0, 0, width, height);
        }

        void clearFaces(void) {
            for ( unsigned i = 0; i < 6; ++i ) {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, this->m_cubemap->get(), 0);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }
        }

        void readyFace(const unsigned faceIndex) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, this->m_cubemap->get(), 0);

            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if ( status != GL_FRAMEBUFFER_COMPLETE ) {
                switch ( status ) {

                case GL_FRAMEBUFFER_UNDEFINED:
                    dalError("Framebuffer status error: GL_FRAMEBUFFER_UNDEFINED"); break;
                case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                    dalError("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
                case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                    dalError("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
                case GL_FRAMEBUFFER_UNSUPPORTED:
                    dalError("Framebuffer status error: GL_FRAMEBUFFER_UNSUPPORTED"); break;
                case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                    dalError("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;

#ifdef _WIN32
                case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                    dalError("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
                case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                    dalError("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
                case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
                    dalError("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;
#endif

                default:
                    dalError(fmt::format("Framebuffer status error: {}", status)); break;

                }
            }
        }

        auto& getCubemap(void) {
            return this->m_cubemap;
        }

    } g_cubemap;

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
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, this->m_bufWidth, this->m_bufHeight, 0, GL_RGB, GL_FLOAT, nullptr);
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

        this->m_tex = new Texture{ this->m_colorMap };
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

    void RenderMaster::MainFramebuffer::clearAndstartRenderOn(void) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_mainFbuf);
        glViewport(0, 0, m_bufWidth, m_bufHeight);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    }

    void RenderMaster::MainFramebuffer::renderOnScreen(const UnilocFScreen& uniloc) {
        glBindVertexArray(m_vbo);

        this->m_tex->sendUniform(uniloc.getTexture());

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

}


// Render Master
namespace dal {

    RenderMaster::RenderMaster(SceneGraph& scene, ShaderMaster& shader, ResourceMaster& resMas,
        ICamera* const camera, const unsigned int winWidth, const unsigned int winHeight)
        : m_scene(scene)
        , m_shader(shader)
        , m_fbuffer(winWidth, winHeight)
        , m_winWidth(winWidth), m_winHeight(winHeight)
        , m_flagDrawDlight1(true)
        , m_skyColor(1.0f, 0.3f, 0.3f)
        , m_mainCamera(camera)
        , m_farPlaneDistance(100.0f)
        , m_baseAmbientColor(0.3f, 0.3f, 0.3f)
    {
        // Lights
        {
            this->m_dlight1.m_color = { 10.0f, 4.f, 4.f };
            this->m_dlight1.setDirectin(0.26373626373626374f, -0.30726256983240224f, 1.f);
            
            this->m_slight1.setColor(5.f, 5.f, 5.f);
            this->m_slight1.setPos(-3.f, 0.f, 0.f);
            this->m_slight1.setDirec(-1, -1, 0);
            this->m_slight1.setEndFadeDegree(30.f);
            this->m_slight1.setStartFadeDegree(25.f);
        }

        // OpenGL global switch
        {
            glClearColor(m_skyColor.x, m_skyColor.y, m_skyColor.z, 1.0f);
#ifdef _WIN32
            glDebugMessageCallback(glDebugCallback, nullptr);
#endif
        }

        // Skybox
        {
            std::array<std::string, 6> cubeMapImages{
                "asset::plane_blue_rt.tga",
                "asset::plane_blue_lf.tga",
                "asset::plane_blue_up.tga",
                "asset::plane_blue_dn.tga",
                "asset::plane_blue_ft.tga",
                "asset::plane_blue_bk.tga",
            };
            this->m_skyboxTex = resMas.orderCubeMap(cubeMapImages, true);
        }

        // Similar to on resize method
        {
            float radio = static_cast<float>(m_winWidth) / static_cast<float>(m_winHeight);
            this->m_projectMat = glm::perspective(glm::radians(90.0f), radio, 0.01f, this->m_farPlaneDistance);

            const auto shorter = this->m_winWidth < this->m_winHeight ? this->m_winWidth : this->m_winHeight;
            if ( shorter > MAX_SCREEN_RES ) {
                auto renderScale = static_cast<float>(MAX_SCREEN_RES) / static_cast<float>(shorter);
                this->resizeRenderScale(renderScale);
            }
        }

        // Init
        {
            g_cubemap.init();
            g_skyRenderer.init();
        }

        // Misc
        {
            const auto renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
            dalVerbose("Graphics device name: {}"_format(renderer));
        }
    }

    void RenderMaster::update(const float deltaTime) {
        const glm::ivec3 intPos{ this->m_mainCamera->m_pos };
        this->m_dlight1.setPos(intPos);

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

    void RenderMaster::render(entt::registry& reg) {
        // Shadow map
        {
#ifdef _WIN32
            glEnable(GL_DEPTH_CLAMP);
#endif

            // Dlight
            {
                this->m_dlight1.clearDepthBuffer();

                {
                    auto& uniloc = this->m_shader.useDepthMp();
                    this->m_dlight1.startRenderShadowmap(uniloc.m_geometry);
                    this->m_scene.renderDepthGeneral(uniloc);
                }

                {
                    auto& uniloc = this->m_shader.useDepthAnime();
                    this->m_dlight1.startRenderShadowmap(uniloc.m_geometry);
                    this->m_scene.renderDepthAnimated(uniloc);
                }

                m_dlight1.finishRenderShadowmap();
            }

            // Slight
            {
                this->m_slight1.clearDepthBuffer();

                {
                    auto& uniloc = this->m_shader.useDepthMp();
                    this->m_slight1.startRenderShadowmap(uniloc.m_geometry);
                    this->m_scene.renderDepthGeneral(uniloc);
                }
                
                {
                    auto& uniloc = this->m_shader.useDepthAnime();
                    this->m_slight1.startRenderShadowmap(uniloc.m_geometry);
                    this->m_scene.renderDepthAnimated(uniloc);
                }

                this->m_slight1.finishRenderShadowmap();
            }
#ifdef _WIN32
            glDisable(GL_DEPTH_CLAMP);
#endif
        }

#ifdef _WIN32
        glEnable(GL_CLIP_DISTANCE0);
#endif

        // Render to water framebuffer
        {
            auto& uniloc = this->m_shader.useGeneral();

            uniloc.m_planeClip.flagDoClip(true);
            uniloc.m_lightedMesh.projectMat(this->m_projectMat);
            uniloc.m_lightedMesh.baseAmbient(this->m_baseAmbientColor);
            uniloc.m_lightedMesh.fogMaxPoint(this->m_farPlaneDistance);
            uniloc.m_lightedMesh.fogColor(this->m_skyColor);

            if ( this->m_flagDrawDlight1 ) {
                this->m_dlight1.sendUniform(uniloc.m_lightedMesh.u_dlights[0]);
                uniloc.m_lightedMesh.dlightCount(1);
            }
            else {
                uniloc.m_lightedMesh.dlightCount(0);
            }

            this->m_slight1.sendUniform(uniloc.m_lightedMesh.u_slights[0]);
            uniloc.m_lightedMesh.slightCount(1);

            this->m_scene.renderOnWaterGeneral(uniloc, *this->m_mainCamera, reg);
        }

        // Render animated to water framebuffer
        {
            auto& uniloc = this->m_shader.useAnimate();

            uniloc.m_planeClip.flagDoClip(true);
            uniloc.m_lightedMesh.projectMat(this->m_projectMat);
            uniloc.m_lightedMesh.baseAmbient(this->m_baseAmbientColor);
            uniloc.m_lightedMesh.fogMaxPoint(this->m_farPlaneDistance);
            uniloc.m_lightedMesh.fogColor(this->m_skyColor);

            if ( this->m_flagDrawDlight1 ) {
                this->m_dlight1.sendUniform(uniloc.m_lightedMesh.u_dlights[0]);
                uniloc.m_lightedMesh.dlightCount(1);
            }
            else {
                uniloc.m_lightedMesh.dlightCount(0);
            }

            this->m_slight1.sendUniform(uniloc.m_lightedMesh.u_slights[0]);
            uniloc.m_lightedMesh.slightCount(1);

            this->m_scene.renderOnWaterAnimated(uniloc, *this->m_mainCamera, reg);
        }

#ifdef _WIN32
        glDisable(GL_CLIP_DISTANCE0);
#endif

        // Render on cube map
        /*{
            const auto lightPos = glm::vec3{ 6, 2, -5 };
            std::vector<glm::mat4> viewMats = {
                glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)),
                glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)),
                glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)),
                glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)),
                glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)),
                glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0))
            };
            const auto projMat = glm::perspective(glm::radians(90.f), 1.f, 0.01f, 1000.f);

            g_cubemap.bindFbuf();
            g_cubemap.clearFaces();

            {
                const auto& uniloc = this->m_shader.useSkybox();

                uniloc.m_geometry.projectMat(projMat);
                uniloc.fogColor(this->m_skyColor);

                for ( unsigned i = 0; i < 6; ++i ) {
                    g_cubemap.readyFace(i);
                    uniloc.m_geometry.viewMat(viewMats[i]);
                    g_skyRenderer.render(uniloc, *this->m_skyboxTex);
                }
            }

            {
                auto& uniloc = this->m_shader.useGeneral();

                uniloc.m_planeClip.flagDoClip(true);
                uniloc.m_lightedMesh.projectMat(projMat);
                uniloc.m_lightedMesh.baseAmbient(this->m_baseAmbientColor);
                uniloc.m_lightedMesh.fogMaxPoint(this->m_farPlaneDistance);
                uniloc.m_lightedMesh.fogColor(this->m_skyColor);

                if ( this->m_flagDrawDlight1 ) {
                    this->m_dlight1.sendUniform(uniloc.m_lightedMesh.u_dlights[0]);
                    uniloc.m_lightedMesh.dlightCount(1);
                }
                else {
                    uniloc.m_lightedMesh.dlightCount(0);
                }

                this->m_slight1.sendUniform(uniloc.m_lightedMesh.u_slights[0]);
                uniloc.m_lightedMesh.slightCount(1);

                for ( unsigned i = 0; i < 6; ++i ) {
                    g_cubemap.readyFace(i);
                    uniloc.m_lightedMesh.viewMat(viewMats[i]);
                    this->m_scene.renderGeneral(uniloc);
                }
            }

            {
                auto& uniloc = this->m_shader.useAnimate();

                uniloc.m_planeClip.flagDoClip(true);
                uniloc.m_lightedMesh.projectMat(glm::perspective(glm::radians(90.f), 1.f, 0.01f, 1000.f));
                uniloc.m_lightedMesh.baseAmbient(this->m_baseAmbientColor);
                uniloc.m_lightedMesh.fogMaxPoint(this->m_farPlaneDistance);
                uniloc.m_lightedMesh.fogColor(this->m_skyColor);

                if ( this->m_flagDrawDlight1 ) {
                    this->m_dlight1.sendUniform(uniloc.m_lightedMesh.u_dlights[0]);
                    uniloc.m_lightedMesh.dlightCount(1);
                }
                else {
                    uniloc.m_lightedMesh.dlightCount(0);
                }

                this->m_slight1.sendUniform(uniloc.m_lightedMesh.u_slights[0]);
                uniloc.m_lightedMesh.slightCount(1);

                for ( unsigned i = 0; i < 6; ++i ) {
                    g_cubemap.readyFace(i);
                    uniloc.m_lightedMesh.viewMat(viewMats[i]);
                    this->m_scene.renderAnimate(uniloc);
                }
            }

            g_cubemap.unbindFbuf(this->m_winWidth, this->m_winHeight);
        }*/

        this->m_fbuffer.clearAndstartRenderOn();

        // Render to framebuffer 
        {
            auto& uniloc = this->m_shader.useStatic();

            uniloc.projMat(this->m_projectMat);
            uniloc.viewMat(this->m_mainCamera->getViewMat());
            uniloc.viewPos(this->m_mainCamera->m_pos);
            uniloc.i_lighting.baseAmbient(this->m_baseAmbientColor);

            if ( this->m_flagDrawDlight1 ) {
                this->m_dlight1.sendUniform(0, uniloc.i_lighting);
                uniloc.i_lighting.dlightCount(1);
            }
            else {
                uniloc.i_lighting.dlightCount(0);
            }

            this->m_scene.render_static(uniloc);
        }

        // Render to framebuffer animated
        {
            auto& uniloc = this->m_shader.useAnimate();

            uniloc.m_lightedMesh.projectMat(this->m_projectMat);
            uniloc.m_planeClip.flagDoClip(false);
            uniloc.m_lightedMesh.viewMat(this->m_mainCamera->getViewMat());
            uniloc.m_lightedMesh.viewPos(this->m_mainCamera->m_pos);
            uniloc.m_lightedMesh.baseAmbient(this->m_baseAmbientColor);
            uniloc.m_lightedMesh.fogMaxPoint(this->m_farPlaneDistance);
            uniloc.m_lightedMesh.fogColor(this->m_skyColor);

            this->m_scene.renderAnimate(uniloc);
        }

        // Render water to framebuffer
        {
            auto& uniloc = this->m_shader.useWaterry();

            uniloc.m_lightedMesh.projectMat(this->m_projectMat);
            uniloc.m_lightedMesh.viewMat(this->m_mainCamera->getViewMat());
            uniloc.m_lightedMesh.viewPos(this->m_mainCamera->m_pos);
            uniloc.m_lightedMesh.baseAmbient(this->m_baseAmbientColor);
            uniloc.m_lightedMesh.fogMaxPoint(this->m_farPlaneDistance);
            uniloc.m_lightedMesh.fogColor(this->m_skyColor);

            if ( this->m_flagDrawDlight1 ) {
                this->m_dlight1.sendUniform(uniloc.m_lightedMesh.u_dlights[0]);
                uniloc.m_lightedMesh.dlightCount(1);
            }
            else {
                uniloc.m_lightedMesh.dlightCount(0);
            }

            this->m_slight1.sendUniform(uniloc.m_lightedMesh.u_slights[0]);
            uniloc.m_lightedMesh.slightCount(1);

            this->m_scene.renderWater(uniloc);
        }

        // Skybox
        {
            const auto& uniloc = this->m_shader.useSkybox();

            uniloc.m_geometry.projectMat(this->m_projectMat);
            const auto skyview = glm::mat4(glm::mat3(this->m_mainCamera->getViewMat()));
            uniloc.m_geometry.viewMat(skyview);
            uniloc.fogColor(this->m_skyColor);

            g_skyRenderer.render(uniloc, *this->m_skyboxTex);
        }

        // Render framebuffer to quad 
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, this->m_winWidth, this->m_winHeight);

            auto& uniloc = this->m_shader.useFScreen();
            this->m_fbuffer.renderOnScreen(uniloc);
        }
    }

    void RenderMaster::resizeRenderScale(const float v) {
        this->m_fbuffer.setRenderScale(v);
        this->m_fbuffer.resizeFbuffer(this->m_winWidth, this->m_winHeight);
    }

    void RenderMaster::onWinResize(const unsigned int width, const unsigned int height) {
        this->m_winWidth = width;
        this->m_winHeight = height;

        float radio = static_cast<float>(width) / static_cast<float>(height);
        this->m_projectMat = glm::perspective(glm::radians(90.0f), radio, 0.01f, this->m_farPlaneDistance);

        const auto shorter = this->m_winWidth < this->m_winHeight ? this->m_winWidth : this->m_winHeight;
        if ( shorter > MAX_SCREEN_RES ) {
            auto renderScale = static_cast<float>(MAX_SCREEN_RES) / static_cast<float>(shorter);
            this->m_fbuffer.setRenderScale(renderScale);
        }

        this->m_fbuffer.resizeFbuffer(width, height);
    }

    ICamera* RenderMaster::replaceMainCamera(ICamera* camera) {
        auto tmp = this->m_mainCamera;
        this->m_mainCamera = camera;
        return tmp;
    }

}
