#include "d_debugview.h"


namespace dal {

    std::vector<DebugViewGod::Triangle>& DebugViewGod::triangles(void) {
        return this->m_triangles;
    }

    const std::vector<DebugViewGod::Triangle>& DebugViewGod::triangles(void) const {
        return this->m_triangles;
    }

    void DebugViewGod::addTriangle(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec4& color) {

#if DAL_DRAW_DEBUG_VIEW
        auto& tri = this->m_triangles.emplace_back();
        tri.m_vert[0] = p0;
        tri.m_vert[1] = p1;
        tri.m_vert[2] = p2;
        tri.m_color = color;
#endif

    }

}
