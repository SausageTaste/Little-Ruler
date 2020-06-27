#include "p_render_master.h"

#include <string>
#include <vector>
#include <array>
#include <memory>

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>

#include <d_logger.h>


#define DAL_RENDER_WATER true


using namespace fmt::literals;


namespace {

    constexpr unsigned MAX_SCREEN_RES = 720;

#ifdef _WIN32
    void GLAPIENTRY glDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
        dalWarn(message);
    }
#endif

}


namespace {

    class VertexBuf_Cube {

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

        void draw(void) const {
            glBindVertexArray(this->m_vao);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

    } g_vertbuf_cube;


    class CubemapFbuf {

    public:
        GLuint m_fbo = 0;
        dal::Texture m_depthMaps[6];

    public:
        CubemapFbuf(void) = default;

        ~CubemapFbuf(void) {
            glDeleteFramebuffers(1, &this->m_fbo);
            this->m_fbo = 0;
        }

        void init(void) {
            glGenFramebuffers(1, &this->m_fbo); dalAssert(0 != this->m_fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, this->m_fbo); dalGLWarn();

            // Depth map
            for ( unsigned i = 0; i < 6; ++i ) {
                this->m_depthMaps[i].init_depthMap(dal::EnvMap::dimension(), dal::EnvMap::dimension());
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0); dalGLWarn();
        }

        void bind(void) {
            glBindFramebuffer(GL_FRAMEBUFFER, this->m_fbo);
        }

        static void unbind(void) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void clearFaceColor(const dal::CubeMap& cubemap, const unsigned faceIndex, const unsigned mip) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, cubemap.get(), mip);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        void readyFace(const unsigned faceIndex, dal::CubeMap& cubemap, const unsigned mip = 0) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, cubemap.get(), mip);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->m_depthMaps[faceIndex].get(), 0);

            const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if ( status != GL_FRAMEBUFFER_COMPLETE ) {
                switch ( status ) {

                case GL_FRAMEBUFFER_UNDEFINED:
                    dalAbort("CubemapFbuf status error: GL_FRAMEBUFFER_UNDEFINED"); break;
                case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                    dalAbort("CubemapFbuf status error: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
                case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                    dalAbort("CubemapFbuf status error: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
                case GL_FRAMEBUFFER_UNSUPPORTED:
                    dalAbort("CubemapFbuf status error: GL_FRAMEBUFFER_UNSUPPORTED"); break;
                case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                    dalAbort("CubemapFbuf status error: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;

#ifdef _WIN32
                case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                    dalAbort("CubemapFbuf status error: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
                case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                    dalAbort("CubemapFbuf status error: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
                case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
                    dalAbort("CubemapFbuf status error: GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;
#endif

                default:
                    dalAbort(fmt::format("CubemapFbuf status error: {}", status)); break;

                }
            }
        }

    } g_cubemapFbuf;


    class VertexBuf_Fillscreen {

    private:
        GLuint m_vbo = 0;
        GLuint m_vertexArr = 0;
        GLuint m_texcoordArr = 0;

    public:
        ~VertexBuf_Fillscreen(void) {
            glDeleteBuffers(1, &this->m_vertexArr);
            glDeleteBuffers(1, &this->m_texcoordArr);
            glDeleteVertexArrays(1, &this->m_vbo);

            this->m_vertexArr = 0;
            this->m_texcoordArr = 0;
            this->m_vbo = 0;
        }

        void init(void) {
            glGenVertexArrays(1, &this->m_vbo); dalAssert(this->m_vbo > 0);
            glGenBuffers(1, &this->m_vertexArr); dalAssert(this->m_vertexArr > 0);
            glGenBuffers(1, &this->m_texcoordArr); dalAssert(this->m_texcoordArr > 0);

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

        void draw(void) const {
            glBindVertexArray(this->m_vbo);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

    } g_vertbuf_fillscreen;


    dal::Texture g_brdfLUT;

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
            glGenFramebuffers(1, &this->m_fbuf);
            glBindFramebuffer(GL_FRAMEBUFFER, this->m_fbuf);

            this->m_colorMap.genTexture("MainFramebuffer::MainFramebuffer");
            glBindTexture(GL_TEXTURE_2D, this->m_colorMap.get());
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, this->m_bufWidth, this->m_bufHeight, 0, GL_RGB, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->m_colorMap.get(), 0);
            glBindTexture(GL_TEXTURE_2D, 0);

            this->m_depthMap.genTexture("MainFramebuffer::MainFramebuffer");
            glBindTexture(GL_TEXTURE_2D, this->m_depthMap.get());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, this->m_bufWidth, this->m_bufHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->m_depthMap.get(), 0);
            glBindTexture(GL_TEXTURE_2D, 0);

            if ( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE ) {
                dalAbort("Failed to create framebuffer.");
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

    RenderMaster::MainFramebuffer::~MainFramebuffer(void) {
        glDeleteFramebuffers(1, &this->m_fbuf);
    }

    void RenderMaster::MainFramebuffer::resizeFbuffer(unsigned int newWin_width, unsigned int newWin_height) {
        this->m_bufWidth = (unsigned int)(float(newWin_width) * this->m_renderScale);
        this->m_bufHeight = (unsigned int)(float(newWin_height) * this->m_renderScale);

        glBindTexture(GL_TEXTURE_2D, this->m_colorMap.get());
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->m_bufWidth, this->m_bufHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindTexture(GL_TEXTURE_2D, this->m_depthMap.get());
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, this->m_bufWidth, this->m_bufHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);

        dalInfo(fmt::format("In-game framebuffer resized to {} x {}.", this->m_bufWidth, this->m_bufHeight));
    }

    void RenderMaster::MainFramebuffer::clearAndstartRenderOn(void) {
        glBindFramebuffer(GL_FRAMEBUFFER, this->m_fbuf);
        glViewport(0, 0, this->m_bufWidth, this->m_bufHeight);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void RenderMaster::MainFramebuffer::sendUniform(const UniRender_FillScreen& uniloc) {
        this->m_colorMap.sendUniform(uniloc.texture());
        this->m_depthMap.sendUniform(uniloc.depthMap());
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
        , m_skyColor(1.0f, 0.3f, 0.3f)
        , m_mainCamera(camera)
        , m_farPlaneDistance(100.0f)
        , m_baseAmbientColor(0.1f)
    {
        // OpenGL global switch
        {
            glClearColor(0, 0, 0, 1);
            glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        }

        // Init
        {
            g_vertbuf_cube.init();
            g_cubemapFbuf.init();
            g_vertbuf_fillscreen.init();
        }

        // Skybox
        {
            std::array<std::string, 6> cubeMapImages{
                "asset::rainbow_rt.png",
                "asset::rainbow_lf.png",
                "asset::rainbow_up.png",
                "asset::rainbow_dn.png",
                "asset::rainbow_ft.png",
                "asset::rainbow_bk.png",
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

        // Generate brdf lut map
        {
            GLuint fbuf = 0;
            glGenFramebuffers(1, &fbuf);
            glBindFramebuffer(GL_FRAMEBUFFER, fbuf);

            g_brdfLUT.genTexture("");
            glBindTexture(GL_TEXTURE_2D, g_brdfLUT.get());
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_brdfLUT.get(), 0);

            glViewport(0, 0, 512, 512);
            this->m_shader.useBrdfLUT();
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            g_vertbuf_fillscreen.draw();

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &fbuf);
        }

        // Misc
        {
            const auto renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
            dalVerbose("Graphics device name: {}"_format(renderer));
        }
    }

    void RenderMaster::update(const float deltaTime) {
        {
            for ( auto& dlight : this->m_scene.m_dlights ) {
                dlight.setPos(glm::round(this->m_mainCamera->m_pos));
            }
        }
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
        this->render_onShadowmaps();
#if DAL_RENDER_WATER
        this->render_onWater(reg);
#endif

        if ( this->m_envmapTimer.getElapsed() >= 1.f ) {
            this->m_envmapTimer.check();
            this->render_onCubemap();
        }

        this->render_onFbuf();

        // Render framebuffer to quad 
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, this->m_winWidth, this->m_winHeight);
            auto& uniloc = this->m_shader.useFillScreen();

            uniloc.projMat(this->m_projectMat);
            uniloc.viewMat(this->m_mainCamera->getViewMat());
            uniloc.viewPos(this->m_mainCamera->m_pos);
            this->m_scene.sendDlightUniform(uniloc.i_lighting);
            this->m_scene.m_mapChunks2.back().sendSlightUniforms(uniloc.i_lighting);

            this->m_fbuffer.sendUniform(uniloc);
            g_vertbuf_fillscreen.draw();
        }
    }

    void RenderMaster::resizeRenderScale(const float v) {
        this->m_fbuffer.setRenderScale(v);
        this->m_fbuffer.resizeFbuffer(this->m_winWidth, this->m_winHeight);
    }

    void RenderMaster::onWinResize(const unsigned int width, const unsigned int height) {
        this->m_winWidth = width;
        this->m_winHeight = height;

        if ( 0 == width || 0 == height )
            return;

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

    // Private

    void RenderMaster::render_onShadowmaps(void) {

#ifdef _WIN32
        glEnable(GL_DEPTH_CLAMP);
#endif

        {
            std::vector<SpotLight*> slights;
            for ( auto& map : this->m_scene.m_mapChunks2 ) {
                for ( auto& l : map.m_slights ) {
                    slights.push_back(&l);
                }
            }

            for ( auto s : slights ) {
                s->clearDepthBuffer();
            }
            for ( auto& d : this->m_scene.m_dlights ) {
                d.clearDepthBuffer();
            }

            {
                auto& uniloc = this->m_shader.useStaticDepth();
                for ( auto s : slights ) {
                    s->startRenderShadowmap(uniloc);
                    this->m_scene.render_staticDepth(uniloc);
                }
                for ( auto& d : this->m_scene.m_dlights ) {
                    d.startRenderShadowmap(uniloc);
                    this->m_scene.render_staticDepth(uniloc);
                }
            }
          
            {
                auto& uniloc = this->m_shader.useAnimatedDepth();
                for ( auto s : slights ) {
                    s->startRenderShadowmap(uniloc);
                    this->m_scene.render_animatedDepth(uniloc);
                }
                for ( auto& d : this->m_scene.m_dlights ) {
                    d.startRenderShadowmap(uniloc);
                    this->m_scene.render_animatedDepth(uniloc);
                }
            }
        }

#ifdef _WIN32
        glDisable(GL_DEPTH_CLAMP);
#endif

    }

    void RenderMaster::render_onWater(entt::registry& reg) {
#ifdef _WIN32
        glEnable(GL_CLIP_DISTANCE0);
#endif

        std::vector<WaterRenderer*> waters;
        for ( auto& m : this->m_scene.m_mapChunks2 ) {
            for ( auto& w : m.m_waters ) {
                waters.push_back(&w);
            }
        }

        // Render to water framebuffer
        {
            auto& uniloc = this->m_shader.useStaticOnWater();

            uniloc.projMat(this->m_projectMat);
            uniloc.viewMat(this->m_mainCamera->getViewMat());
            uniloc.viewPos(this->m_mainCamera->m_pos);
            uniloc.i_lighting.baseAmbient(this->m_baseAmbientColor);

            for ( auto water : waters ) {
                {
                    water->startRenderOnReflec(uniloc, *this->m_mainCamera);
                    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
                    this->m_scene.render_staticOnWater(uniloc);
                }

                {
                    water->startRenderOnRefrac(uniloc, *this->m_mainCamera);
                    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
                    this->m_scene.render_staticOnWater(uniloc);
                }
            }
        }

        // Render animated to water framebuffer
        {
            auto& uniloc = this->m_shader.useAnimatedOnWater();

            uniloc.projMat(this->m_projectMat);
            uniloc.viewMat(this->m_mainCamera->getViewMat());
            uniloc.viewPos(this->m_mainCamera->m_pos);
            uniloc.i_lighting.baseAmbient(this->m_baseAmbientColor);

            for ( auto water : waters ) {
                {
                    water->startRenderOnReflec(uniloc, *this->m_mainCamera);
                    this->m_scene.render_animatedOnWater(uniloc);
                }

                {
                    water->startRenderOnRefrac(uniloc, *this->m_mainCamera);
                    this->m_scene.render_animatedOnWater(uniloc);
                }
            }
        }

#ifdef _WIN32
        glDisable(GL_CLIP_DISTANCE0);
#endif

        {
            const auto& uniloc = this->m_shader.useSkybox();
            const float sqrt2 = sqrt(1.0 / 3.0);

            this->m_skyboxTex->sendUniform(uniloc.skyboxTex());

            uniloc.modelMat(glm::scale(glm::mat4{ 1 }, glm::vec3{ sqrt2 * this->m_farPlaneDistance }));
            uniloc.viewPos(0, 0, 0);

            glDepthMask(GL_FALSE);
            glDepthFunc(GL_LEQUAL);

            for ( auto water : waters ) {
                water->startRenderOnReflec(uniloc, *this->m_mainCamera, this->m_projectMat);
                g_vertbuf_cube.draw();
            }

            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);
        }

    }

    void RenderMaster::render_onCubemap(void) {
        const auto projMat = glm::perspective(glm::radians(90.f), 1.f, 0.5f, 50.f);

        g_cubemapFbuf.bind();

        for ( auto& m : this->m_scene.m_mapChunks2 ) {
            for ( auto& e : m.m_envmap ) {
                std::array<glm::mat4, 6> viewMats = {
                    glm::lookAt(e.m_pos, e.m_pos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)),
                    glm::lookAt(e.m_pos, e.m_pos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)),
                    glm::lookAt(e.m_pos, e.m_pos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)),
                    glm::lookAt(e.m_pos, e.m_pos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)),
                    glm::lookAt(e.m_pos, e.m_pos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)),
                    glm::lookAt(e.m_pos, e.m_pos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0))
                };

                glViewport(0, 0, e.dimension(), e.dimension());
                for ( unsigned i = 0; i < 6; ++i ) {
                    g_cubemapFbuf.clearFaceColor(e.cubemap(), i, 0);
                }

                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LESS);
                glEnable(GL_CULL_FACE);

                // Static
                {
                    auto& uniloc = this->m_shader.useStatic();

                    uniloc.projMat(projMat);
                    uniloc.viewPos(e.m_pos);
                    uniloc.i_lighting.baseAmbient(this->m_baseAmbientColor);
                    uniloc.i_envmap.envmap().setFlagHas(false);
                    g_brdfLUT.sendUniform(uniloc.i_envmap.brdfLUT());

                    for ( unsigned i = 0; i < 6; ++i ) {
                        g_cubemapFbuf.readyFace(i, e.cubemap());
                        glClear(GL_DEPTH_BUFFER_BIT);
                        uniloc.viewMat(viewMats[i]);
                        this->m_scene.render_staticOnEnvmap(uniloc);
                    }
                }

                // Animated
                {
                    auto& uniloc = this->m_shader.useAnimated();

                    uniloc.projMat(projMat);
                    uniloc.viewPos(e.m_pos);
                    uniloc.i_lighting.baseAmbient(this->m_baseAmbientColor);
                    uniloc.i_envmap.envmap().setFlagHas(false);
                    g_brdfLUT.sendUniform(uniloc.i_envmap.brdfLUT());

                    for ( unsigned i = 0; i < 6; ++i ) {
                        g_cubemapFbuf.readyFace(i, e.cubemap());
                        uniloc.viewMat(viewMats[i]);
                        this->m_scene.render_animated(uniloc);
                    }
                }

                glDepthFunc(GL_LEQUAL);

                // Skybox
                {
                    const auto& uniloc = this->m_shader.useSkybox();
                    const float sqrt2 = sqrt(1.0 / 3.0);

                    this->m_skyboxTex->sendUniform(uniloc.skyboxTex());
                    uniloc.viewPosActual(this->m_mainCamera->m_pos);
                    uniloc.modelMat(glm::scale(glm::mat4{ 1 }, glm::vec3{ sqrt2 * this->m_farPlaneDistance }));
                    uniloc.viewPos(0, 0, 0);

                    for ( unsigned i = 0; i < 6; ++i ) {
                        g_cubemapFbuf.readyFace(i, e.cubemap());
                        uniloc.projMat(projMat);
                        uniloc.viewMat(glm::mat4{ glm::mat3{ viewMats[i] } });
                        g_vertbuf_cube.draw();
                    }
                }

                glDepthFunc(GL_ALWAYS);

                // Irradiance
                {
                    auto& uniloc = this->m_shader.useCubeIrradiance();

                    uniloc.projMat(projMat);
                    e.cubemap().sendUniform(uniloc.envmap());

                    for ( unsigned i = 0; i < 6; ++i ) {
                        g_cubemapFbuf.readyFace(i, e.irradianceMap());
                        glClear(GL_COLOR_BUFFER_BIT);
                        uniloc.viewMat(glm::mat4{ glm::mat3{viewMats[i]} });
                        g_vertbuf_cube.draw();
                    }
                }

                // Prefilter
                {
                    auto& uniloc = this->m_shader.useCubePrefilter();

                    uniloc.projMat(projMat);
                    e.cubemap().sendUniform(uniloc.envmap());

                    constexpr unsigned MAX_MIP_LVL = 4;
                    for ( unsigned mip = 0; mip <= MAX_MIP_LVL; ++mip ) {
                        const unsigned mipDimension  = e.dimension() * std::pow(0.5, mip);
                        glViewport(0, 0, mipDimension, mipDimension);

                        const float roughness = static_cast<float>(mip) / static_cast<float>(MAX_MIP_LVL);
                        uniloc.roughness(roughness);

                        for ( unsigned i = 0; i < 6; ++i ) {
                            g_cubemapFbuf.readyFace(i, e.prefilterMap(), mip);
                            glClear(GL_COLOR_BUFFER_BIT);
                            uniloc.viewMat(glm::mat4{ glm::mat3{viewMats[i]} });
                            g_vertbuf_cube.draw();
                        }
                    }
                }
            }
        }

        g_cubemapFbuf.unbind();
        glViewport(0, 0, this->m_winWidth, this->m_winHeight);
        glDepthFunc(GL_LESS);
    }

    void RenderMaster::render_onFbuf(void) {
        this->m_fbuffer.clearAndstartRenderOn();

        // Render to framebuffer 
        {
            auto& uniloc = this->m_shader.useStatic();

            uniloc.projMat(this->m_projectMat);
            uniloc.viewMat(this->m_mainCamera->getViewMat());
            uniloc.viewPos(this->m_mainCamera->m_pos);
            uniloc.i_lighting.baseAmbient(this->m_baseAmbientColor);
            uniloc.i_envmap.envmap().setFlagHas(false);
            g_brdfLUT.sendUniform(uniloc.i_envmap.brdfLUT());
            //g_cubemapFbuf.m_depthMaps[0].sendUniform(uniloc.i_envmap.brdfLUT());

            this->m_scene.render_static(uniloc);
        }

        // Render to framebuffer animated
        {
            auto& uniloc = this->m_shader.useAnimated();

            uniloc.projMat(this->m_projectMat);
            uniloc.viewMat(this->m_mainCamera->getViewMat());
            uniloc.viewPos(this->m_mainCamera->m_pos);
            uniloc.i_lighting.baseAmbient(this->m_baseAmbientColor);
            uniloc.i_envmap.envmap().setFlagHas(false);
            g_brdfLUT.sendUniform(uniloc.i_envmap.brdfLUT());

            this->m_scene.render_animated(uniloc);
        }

        // Render water to framebuffer
#if DAL_RENDER_WATER
        {
            auto& uniloc = this->m_shader.useWater();

            uniloc.projMat(this->m_projectMat);
            uniloc.viewMat(this->m_mainCamera->getViewMat());
            uniloc.viewPos(this->m_mainCamera->m_pos);
            uniloc.i_lighting.baseAmbient(this->m_baseAmbientColor);
            this->m_scene.sendDlightUniform(uniloc.i_lighting);

            for ( auto& m : this->m_scene.m_mapChunks2 ) {
                m.renderWater(uniloc);
            }
        }
#endif

        // Skybox
        {
            const auto& uniloc = this->m_shader.useSkybox();
            const float sqrt2 = sqrt(1.0 / 3.0);

            this->m_skyboxTex->sendUniform(uniloc.skyboxTex());
            uniloc.viewPosActual(this->m_mainCamera->m_pos);

            uniloc.projMat(this->m_projectMat);
            uniloc.viewMat(glm::mat4(glm::mat3(this->m_mainCamera->getViewMat())));
            uniloc.modelMat(glm::scale(glm::mat4{ 1 }, glm::vec3{ sqrt2 * this->m_farPlaneDistance }));
            uniloc.viewPos(0, 0, 0);

            glDepthMask(GL_FALSE);
            glDepthFunc(GL_LEQUAL);

            g_vertbuf_cube.draw();

            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);
        }
    }

}
