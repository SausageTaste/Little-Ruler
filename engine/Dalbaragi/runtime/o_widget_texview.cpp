#include "o_widget_texview.h"


namespace dal {

    TextureView::TextureView(Widget2* parent, const Texture* const tex)
        : Widget2(parent)
        , m_tex(tex)
        , m_upsideDown(false)
    {

    }

    void TextureView::render(const UniRender_Overlay& uniloc, const float width, const float height) {
        QuadRenderInfo info;

        std::tie(info.m_bottomLeftNormalized, info.m_rectSize) = this->makePosSize(width, height);
        info.m_color = glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f };
        info.m_diffuseMap = this->m_tex;
        info.m_upsideDown_diffuse = this->m_upsideDown;

        renderQuadOverlay(uniloc, info);
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

    void ColoredTile::render(const UniRender_Overlay& uniloc, const float width, const float height) {
        QuadRenderInfo info;

        std::tie(info.m_bottomLeftNormalized, info.m_rectSize) = this->makePosSize(width, height);
        info.m_color = this->m_color;

        renderQuadOverlay(uniloc, info);
    }

    void ColoredTile::setColor(const float r, const float g, const float b, const float a) {
        this->m_color = glm::vec4{ r, g, b, a };
    }

}