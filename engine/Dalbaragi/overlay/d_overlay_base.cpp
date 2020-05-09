#include "d_overlay_base.h"


namespace {

    void flipAxis(glm::vec2& scale, glm::vec2& offset, unsigned index) {
        const auto new_scale = -scale[index];
        const auto new_offset = offset[index] + scale[index];

        scale[index] = new_scale;
        offset[index] = new_offset;
    }

}


namespace dal {

    void OverlayDrawInfo::flipX(void) {
        flipAxis(this->m_texScale, this->m_texOffset, 0);
    }

    void OverlayDrawInfo::flipY(void) {
        flipAxis(this->m_texScale, this->m_texOffset, 1);
    }

}
