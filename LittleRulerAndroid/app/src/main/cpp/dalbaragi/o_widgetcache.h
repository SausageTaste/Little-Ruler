#pragma once

#include "p_dalopengl.h"
#include "p_meshStatic.h"


namespace dal {

    class WidgetCache {

    private:
        class Framebuffer {

        private:
            unsigned int m_width, m_height;
            GLuint m_fbo = 0;
            Texture m_colorMap;

        public:
            Framebuffer(const unsigned int width, const unsigned int height);

            void resize(const unsigned int width, const unsigned int height);
            void clearAndstartRenderOn(void) const;

            const Texture& getTexture(void) const {
                return this->m_colorMap;
            }

        };

    private:
        Framebuffer m_fbuf;

    public:
        WidgetCache(const unsigned int width, const unsigned int height);

        void resize(const unsigned int width, const unsigned int height) {
            this->m_fbuf.resize(width, height);
        }
        void clearAndstartRenderOn(void) const {
            this->m_fbuf.clearAndstartRenderOn();
        }
        const Texture& getTexture(void) const {
            return this->m_fbuf.getTexture();
        }
        
    };

}