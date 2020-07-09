#pragma once

#include <array>
#include <vector>

#include <glm/glm.hpp>


#define DAL_DRAW_DEBUG_VIEW true


namespace dal {

    class DebugViewGod {

    public:
        struct Triangle {
            std::array<glm::vec3, 3> m_vert;
            glm::vec4 m_color{ 1 };
        };

    private:
        std::vector<Triangle> m_triangles;

    public:
        DebugViewGod(const DebugViewGod&) = delete;
        DebugViewGod& operator=(const DebugViewGod&) = delete;
        DebugViewGod(DebugViewGod&&) = delete;
        DebugViewGod& operator=(DebugViewGod&&) = delete;

    public:
        DebugViewGod(void) = default;

        static DebugViewGod& inst(void) {
            static DebugViewGod i;
            return i;
        }

        auto triangles(void) -> std::vector<Triangle>&;
        auto triangles(void) const -> const std::vector<Triangle>&;
        void addTriangle(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec4& color);

    };

}
