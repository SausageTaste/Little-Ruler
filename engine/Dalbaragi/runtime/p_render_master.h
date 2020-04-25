#pragma once

#include <list>

#include <glm/glm.hpp>
#include <entt/entity/registry.hpp>

#include "p_scene.h"
#include "p_shader_master.h"


namespace dal {

    class RenderMaster {

    private:
        class MainFramebuffer {
            float m_renderScale;
            unsigned int m_bufWidth, m_bufHeight;

            GLuint m_mainFbuf = 0;
            GLuint m_colorMap = 0;
            GLuint m_mainRenderbuf = 0;
            GLuint m_vbo = 0;
            GLuint m_vertexArr = 0;
            GLuint m_texcoordArr = 0;

            Texture* m_tex = nullptr;

        public:
            MainFramebuffer(const unsigned int widWidth, const unsigned int widHeight);
            ~MainFramebuffer(void);

            void setRenderScale(const float v) {
                this->m_renderScale = v;
            }
            void resizeFbuffer(const unsigned int w, const unsigned int h);

            void clearAndstartRenderOn(void);
            void renderOnScreen(const UniRender_FillScreen& uniloc);

        };

    private:
        SceneGraph& m_scene;
        ShaderMaster& m_shader;

        MainFramebuffer m_fbuffer;

        unsigned int m_winWidth, m_winHeight;
        glm::mat4 m_projectMat;

        DirectionalLight m_dlight1;
        bool m_flagDrawDlight1;

        glm::vec3 m_skyColor;
        ICamera* m_mainCamera;

        float m_farPlaneDistance;
        glm::vec3 m_baseAmbientColor;

        std::shared_ptr<const CubeMap> m_skyboxTex;

    public:
        RenderMaster(SceneGraph& scene, ShaderMaster& shader, ResourceMaster& resMas,
            ICamera* const camera, const unsigned int winWidth, const unsigned int winHeight);

        void update(const float deltaTime);
        void render(entt::registry& reg);
        void resizeRenderScale(const float v);

        void onWinResize(const unsigned int width, const unsigned int height);

        ICamera* replaceMainCamera(ICamera* camera);

    private:
        void render_onShadowmaps(void);
        void render_onWater(entt::registry& reg);
        void render_onCubemap(void);
        void render_onFbuf(void);

    };

}