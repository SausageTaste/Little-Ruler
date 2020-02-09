#pragma once

#include <glm/glm.hpp>

#include "u_loadinfo.h"
#include "p_dalopengl.h"
#include "p_meshStatic.h"
#include "p_uniloc.h"
//#include "p_resource.h"
#include "u_timer.h"


namespace dal {

    void deleteFramebuffer(const GLuint fbo);
    void deleteRenderbuffer(const GLuint rbo);


    template <void (*DEL_FUNC)(GLuint)>
    class GLObject {

    private:
        static constexpr GLuint s_nullValue = 0;
        GLuint m_fbo;

    private:
        GLObject(const GLObject&) = delete;
        GLObject& operator=(const GLObject&) = delete;

    public:
        GLObject(void) : m_fbo(s_nullValue) {

        }

        GLObject(GLObject&& other) noexcept {
            this->m_fbo = other.m_fbo;
            other.m_fbo = s_nullValue;
        }

        GLObject& operator=(GLObject&& other) noexcept {
            this->m_fbo = other.m_fbo;
            other.m_fbo = s_nullValue;
            return *this;
        }

        ~GLObject(void) {
            this->del();
        }

        void reset(const GLuint fbo) {
            this->del();
            this->m_fbo = fbo;
        }

        GLuint get(void) const {
            assert(this->isReady());
            return this->m_fbo;
        }

        bool isReady(void) const {
            return this->m_fbo != s_nullValue;
        }

        void del(void) {
            if ( this->isReady() ) {
                DEL_FUNC(this->m_fbo);
                this->m_fbo = 0;
            }
        }

    };


    class WaterFramebuffer {

    private:
        GLObject<deleteFramebuffer> m_reflectionFrameBuffer;
        Texture m_reflectionTexture;
        GLObject<deleteRenderbuffer> m_reflectionDepthBuffer;

        GLObject<deleteFramebuffer> m_refractionFrameBuffer;
        Texture m_refractionTexture;
        Texture m_refractionDepthTexture;

        float m_winWidth, m_winHeight;
        float m_reflecScale, m_refracScale;

    public:
        WaterFramebuffer(const unsigned int winWidth, const unsigned int winHeight);

        void sendUniform(const UnilocWaterry& uniloc) const;
        void bindReflectionFrameBuffer(void) const;
        void bindRefractionFrameBuffer(void) const;

        void onWinResize(const unsigned int winWidth, const unsigned int winHeight);

    private:
        void resizeOnlyTextures(const unsigned int reflecWidth, const unsigned int reflecHeight, const unsigned int refracWidth, const unsigned int refracHeight);
        void recreateFbuffer(const unsigned int reflecWidth, const unsigned int reflecHeight, const unsigned int refracWidth, const unsigned int refracHeight);

    };


    class WaterRenderer {

    private:
        Material m_material;
        MeshStatic m_mesh;
        WaterFramebuffer m_fbuffer;
        Timer m_localTimer;
        glm::vec3 m_depthColor;

        float m_height;
        float m_moveSpeed;
        float m_waveStreng;
        float m_darkestDepthPoint;
        float m_reflectivity;
        float m_moveFactor;

        dal::Texture *m_dudvMap, *m_normalMap;

    public:
        WaterRenderer(const dlb::WaterPlane& info, const unsigned int winWidth, const unsigned int winHeight);

        void render(const UnilocWaterry& uniloc);

        void startRenderOnReflec(const UniRender_StaticOnWater& uniloc, const ICamera& cam) const;
        void startRenderOnReflec(const UniRender_AnimatedOnWater& uniloc, const ICamera& cam) const;
        void startRenderOnReflec(const UniInterfGeometry& uniloc, const ICamera& cam) const;

        void startRenderOnRefrac(const UniRender_StaticOnWater& uniloc, const ICamera& cam) const;
        void startRenderOnRefrac(const UniRender_AnimatedOnWater& uniloc, const ICamera& cam) const;

        void onWinResize(const unsigned int winWidth, const unsigned int winHeight) {
            this->m_fbuffer.onWinResize(winWidth, winHeight);
        }

    private:
        void initMesh(const glm::vec3& pos, const glm::vec2& size);

    };

}
