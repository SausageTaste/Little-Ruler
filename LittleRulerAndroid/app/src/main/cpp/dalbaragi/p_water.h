#pragma once

#include <glm/glm.hpp>

#include "p_dalopengl.h"
#include "p_meshStatic.h"
#include "p_uniloc.h"
#include "p_resource.h"
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

        GLuint get(void) {
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
        GLObject<deleteRenderbuffer> m_refractionDepthTexture;

        float m_winWidth, m_winHeight;
        float m_reflecScale, m_refracScale;

    public:
        WaterFramebuffer(const unsigned int winWidth, const unsigned int winHeight);

        void bindReflectionFrameBuffer(void);
        void bindRefractionFrameBuffer(void);

        Texture* getReflectionTexture(void);
        Texture* getRefractionTexture(void);
        GLuint getRefractionDepthTexture(void);

        void resizeFbuffer(const unsigned int winWidth, const unsigned int winHeight);

    };


    class WaterRenderer {

    private:
        MeshStatic m_mesh;
        Material m_material;
        float m_height = 0.0f;
        float m_moveFactor = 0.0f;
        float m_moveSpeed = 0.03f;
        Timer m_localTimer;

    public:
        WaterFramebuffer m_fbuffer;

    public:
        WaterRenderer(const glm::vec3& pos, const glm::vec2& size, const unsigned int winWidth, const unsigned int winHeight);
        void renderWaterry(const UnilocWaterry& uniloc);
        float getHeight(void) const;

    };

}