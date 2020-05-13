#include "d_widget_view.h"


namespace dal {

    void ColorView::render(const float width, const float height, const void* userData) {
        OverlayDrawInfo info;

        info.m_pos = screen2device(this->aabb().point01(), width, height);
        info.m_dimension = size2device(this->aabb().size(), glm::vec2{ width, height });
        info.m_color = this->m_color;

        this->draw(info, userData);
    }

}
