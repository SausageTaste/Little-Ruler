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

        private:
            float m_renderScale;
            unsigned int m_bufWidth, m_bufHeight;

            GLuint m_fbuf = 0;
            Texture m_colorMap, m_depthMap;

        public:
            MainFramebuffer(const unsigned int widWidth, const unsigned int widHeight);
            ~MainFramebuffer(void);

            void setRenderScale(const float v) {
                this->m_renderScale = v;
            }
            void resizeFbuffer(const unsigned int w, const unsigned int h);

            void clearAndstartRenderOn(void);
            void sendUniform(const UniRender_FillScreen& uniloc);

        };

    private:
        SceneGraph& m_scene;
        ShaderMaster& m_shader;

        MainFramebuffer m_fbuffer;

        unsigned int m_winWidth, m_winHeight;
        glm::mat4 m_projectMat;

        glm::vec3 m_skyColor;
        ICamera* m_mainCamera;

        float m_farPlaneDistance;
        glm::vec3 m_baseAmbientColor;

        dal::Timer m_envmapTimer;

        std::shared_ptr<const CubeMap> m_skyboxTex;

    public:
        RenderMaster(SceneGraph& scene, ShaderMaster& shader, ResourceMaster& resMas,
            ICamera* const camera, const unsigned int winWidth, const unsigned int winHeight);

        auto& projMat(void) const {
            return this->m_projectMat;
        }

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
