#pragma once

#include "d_widget.h"


namespace dal {

    class ColorView : public Widget2D {

    public:
        glm::vec4 m_color;

    public:
        using Widget2D::Widget2D;

        virtual void render(const float width, const float height, const void* userData) override;

    };

}
