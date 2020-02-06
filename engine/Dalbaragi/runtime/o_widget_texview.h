#pragma once

#include "p_resource.h"
#include "o_widgetbase.h"


namespace dal {

    class TextureView : public Widget2 {

    private:
        const Texture* m_tex;
        bool m_upsideDown;

    public:
        TextureView(Widget2* parent, const Texture* const tex = nullptr);

        virtual void render(const UnilocOverlay& uniloc, const float width, const float height) override;

        void setTexture(Texture* const tex);
        void setUpsideDown(const bool v);

    };


    class ColoredTile : public Widget2 {

    private:
        glm::vec4 m_color;

    public:
        ColoredTile(Widget2* const parent, const float r, const float g, const float b, const float a);

        virtual void render(const UnilocOverlay& uniloc, const float width, const float height) override;

        void setColor(const float r, const float g, const float b, const float a);

    };

}