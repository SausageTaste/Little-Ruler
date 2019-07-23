#include "o_widget_texview.h"


namespace dal {

    TextureView::TextureView(Widget2* parent, const Texture* const tex)
        : Widget2(parent)
        , m_tex(nullptr)
        , m_upsideDown(false)
    {

    }

    void TextureView::render(const UnilocOverlay& uniloc, const float width, const float height) {
        renderQuadOverlay(uniloc, this->makeDeviceSpace(width, height), glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f }, this->m_tex, nullptr, this->m_upsideDown, false);
    }

    void TextureView::setTexture(Texture* const tex) {
        this->m_tex = tex;
    }

    void TextureView::setUpsideDown(const bool v) {
        this->m_upsideDown = v;
    }

}


namespace dal {

    ColoredTile::ColoredTile(Widget2* const parent, const float r, const float g, const float b, const float a)
        : Widget2(parent)
    {
        this->setColor(r, g, b, a);
    }

    void ColoredTile::render(const UnilocOverlay& uniloc, const float width, const float height) {
        renderQuadOverlay(uniloc, this->makeDeviceSpace(width, height), this->m_color, nullptr, nullptr, false, false);
    }

    void ColoredTile::setColor(const float r, const float g, const float b, const float a) {
        this->m_color = glm::vec4{ r, g, b, a };
    }

}