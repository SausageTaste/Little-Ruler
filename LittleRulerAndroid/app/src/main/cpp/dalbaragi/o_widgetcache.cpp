#include "o_widgetcache.h"

#include <d_logger.h>


namespace dal {

    WidgetCache::Framebuffer::Framebuffer(const unsigned int width, const unsigned int height)
        : m_width(width)
        , m_height(height)
    {
        glGenFramebuffers(1, &this->m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, this->m_fbo);

        this->m_colorMap.initAttach_colorMap(this->m_width, this->m_height);
        glBindTexture(GL_TEXTURE_2D, 0);

        if ( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE ) {
            dalAbort("Failed to create framebuffer.");
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void WidgetCache::Framebuffer::resize(const unsigned int width, const unsigned int height) {
        this->m_width = width;
        this->m_height = height;
        this->m_colorMap.resize_colorMap(width, height);
    }

    void WidgetCache::Framebuffer::clearAndstartRenderOn(void) const {
        glBindFramebuffer(GL_FRAMEBUFFER, this->m_fbo);
        glViewport(0, 0, this->m_width, this->m_height);
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

}


namespace dal {

    WidgetCache::WidgetCache(const unsigned int width, const unsigned int height)
        : m_fbuf(width, height)
    {

    }

}