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

        virtual void render(const UniRender_Overlay& uniloc, const float width, const float height) override;

        void setTexture(Texture* const tex);
        void setUpsideDown(const bool v);

    };

}