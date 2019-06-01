#pragma once

#include "o_widget_base.h"
#include "p_dalopengl.h"
#include "p_resource.h"


namespace dal {

    class TextureView : public Widget {

    private:
        QuadRenderer m_quadRender;

    public:
        TextureView(Widget* parent, Texture* const tex = nullptr);
        virtual void renderOverlay(const UnilocOverlay& uniloc) override;
        void setTexture(Texture* const tex);

        void setUpsideDown(const bool v);

    };


    class ColoredTile : public Widget {

    private:
        QuadRenderer m_quadRender;

    public:
        ColoredTile(Widget* const parent, const float r, const float g, const float b, const float a);
        virtual void renderOverlay(const UnilocOverlay& uniloc) override;
        void setColor(const float r, const float g, const float b, const float a);

    };

}