#pragma once

#include <list>

#include <glm/glm.hpp>


#include "s_event.h"

#include "p_shader_master.h"
#include "p_uniloc.h"
#include "p_meshStatic.h"
#include "p_light.h"
#include "o_overlay_master.h"
#include "p_dalopengl.h"
#include "p_scene.h"
#include "g_actor.h"
#include "p_water.h"


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

            void setRenderScale(float v, unsigned int widWidth, unsigned int widHeight);
            void resizeFbuffer(unsigned int w, unsigned int h);

            void startRenderOn(void);
            void renderOnScreen(const UnilocFScreen& uniloc);

            //Texture* getTex(void);

        };

    private:
        SceneMaster& m_scene;
        ShaderMaster& m_shader;

        MainFramebuffer m_fbuffer;

        unsigned int m_winWidth, m_winHeight;
        glm::mat4 m_projectMat;

        DirectionalLight m_dlight1;
        bool m_flagDrawDlight1;

        glm::vec3 m_skyColor;
        ICamera* m_mainCamera;

    public:
        RenderMaster(SceneMaster& scene, ShaderMaster& shader, OverlayMaster& overlay, ICamera* const camera, const unsigned int winWidth, const unsigned int winHeight);

        void update(const float deltaTime);
        void render(void);
        void setRenderScale(float v);

        void onWinResize(const unsigned int width, const unsigned int height);

        ICamera* replaceMainCamera(ICamera* camera);

    };

}