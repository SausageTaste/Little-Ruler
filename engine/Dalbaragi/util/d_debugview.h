#pragma once

#include <array>
#include <vector>

#include <glm/glm.hpp>


namespace dal {

    class DebugViewGod {

    public:
        struct Triangle {
            std::array<glm::vec3, 3> m_vert;
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

        auto& triangles(void) {
            return this->m_triangles;
        }
        auto& triangles(void) const {
            return this->m_triangles;
        }
        void addTriangle(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) {
            auto& tri = this->m_triangles.emplace_back();
            tri.m_vert[0] = p0;
            tri.m_vert[1] = p1;
            tri.m_vert[2] = p2;
        }

    };

}
