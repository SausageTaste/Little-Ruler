#pragma once

#include <vector>
#include <array>

#include <glm/glm.hpp>

#include "d_daldef.h"
#include "d_geometrymath.h"


namespace dal {

    class MeshBuildData {

    private:
        std::vector<float> m_vert, m_texc, m_norm;

    public:
        void reserve(const size_t numVert);
        void addVertex(const glm::vec3& vert, const glm::vec2& texcoord, const glm::vec3& normal);

        size_t numVert(void) const;
        const float* vertices(void) const {
            return this->m_vert.data();
        }
        const float* texcoords(void) const {
            return this->m_texc.data();
        }
        const float* normals(void) const {
            return this->m_norm.data();
        }

    };


    class MeshData {

    private:
        struct Face {
            constexpr static int VERT_PER_FACE = 3;
            std::array<size_t, VERT_PER_FACE> m_verts;
            std::array<xvec2, VERT_PER_FACE> m_texcoords = { {xvec2{0, 0}, xvec2{0, 0}, xvec2{0, 0}} };
        };

    private:
        std::vector<xvec3> m_vertices;
        std::vector<Face> m_faces;

    public:
        MeshBuildData buildMesh(void) const;

        void addQuad(const xvec3& p0, const xvec3& p1, const xvec3& p2, const xvec3& p3);

        bool isIntersecting(const Segment& seg, const glm::mat4& transform) const;
        std::optional<SegIntersecInfo> findIntersection(const Segment& seg, const glm::mat4& transform) const;

    private:
        xvec3 makeFaceNormal(const Face& face) const;
        std::vector<glm::vec3> makeTransformedVert(const glm::mat4& transform) const;

    };

}
