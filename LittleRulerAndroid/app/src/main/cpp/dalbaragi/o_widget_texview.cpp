#include "o_widget_texview.h"


namespace dal {

    TextureView::TextureView(Widget* parent, Texture* const tex)
        : Widget(parent)
    {
        this->m_quadRender.setColor(0.0, 0.0, 0.0, 1.0f);
        this->m_quadRender.setDiffuseMap(tex);
    }

    void TextureView::renderOverlay(const UnilocOverlay& uniloc) {
        this->m_quadRender.renderQuad(uniloc, this->getDeviceSpace());
    }

    void TextureView::setTexture(Texture* const tex) {
        this->m_quadRender.setDiffuseMap(tex);
    }

    void TextureView::setUpsideDown(const bool v) {
        this->m_quadRender.setUpsideDown_diffuseMap(v);
    }

}