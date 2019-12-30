#include "d_meshgeo.h"

#include <limits>


// MeshBuildData
namespace dal {

    void MeshBuildData::reserve(const size_t numVert) {
        this->m_vert.reserve(numVert * 3);
        this->m_texc.reserve(numVert * 2);
        this->m_norm.reserve(numVert * 3);
    }

    void MeshBuildData::addVertex(const glm::vec3& vert, const glm::vec2& texcoord, const glm::vec3& normal) {
        this->m_vert.push_back(vert.x);
        this->m_vert.push_back(vert.y);
        this->m_vert.push_back(vert.z);

        this->m_texc.push_back(texcoord.x);
        this->m_texc.push_back(texcoord.y);

        this->m_norm.push_back(normal.x);
        this->m_norm.push_back(normal.y);
        this->m_norm.push_back(normal.z);
    }

    size_t MeshBuildData::numVert(void) const {
        const size_t numVert = this->m_vert.size() / 3;

        assert(numVert * 3 == this->m_vert.size());
        assert(numVert * 2 == this->m_texc.size());
        assert(numVert * 3 == this->m_norm.size());

        return numVert;
    }

}


// MeshData
namespace dal {

    MeshBuildData MeshData::buildMesh(void) const {
        MeshBuildData result;

        result.reserve(this->m_faces.size() * 3);

        for ( auto& face : this->m_faces ) {
            const auto normal = this->makeFaceNormal(face);

            for ( int i = 0; i < 3; ++i ) {
                result.addVertex(this->m_vertices[face.m_verts[i]], face.m_texcoords[i], normal);
            }
        }

        return result;
    }

    void MeshData::addQuad(const xvec3& p0, const xvec3& p1, const xvec3& p2, const xvec3& p3) {
        const auto startIndex = this->m_vertices.size();

        this->m_vertices.push_back(p0);
        this->m_vertices.push_back(p1);
        this->m_vertices.push_back(p2);
        this->m_vertices.push_back(p3);

        {
            Face* face0 = &this->m_faces.emplace_back();

            face0->m_verts[0] = startIndex + 0;
            face0->m_verts[1] = startIndex + 1;
            face0->m_verts[2] = startIndex + 2;
            face0->m_texcoords[0] = xvec2{ 0, 1 };
            face0->m_texcoords[1] = xvec2{ 0, 0 };
            face0->m_texcoords[2] = xvec2{ 1, 0 };
        }

        {
            Face& face1 = this->m_faces.emplace_back();

            face1.m_verts[0] = startIndex + 0;
            face1.m_verts[1] = startIndex + 2;
            face1.m_verts[2] = startIndex + 3;
            face1.m_texcoords[0] = xvec2{ 0, 1 };
            face1.m_texcoords[1] = xvec2{ 1, 0 };
            face1.m_texcoords[2] = xvec2{ 1, 1 };
        }
    }

    bool MeshData::isIntersecting(const Segment& seg, const glm::mat4& transform) const {
        const auto transformed = this->makeTransformedVert(transform);

        for ( const auto& face : this->m_faces ) {
            const auto v0 = transformed[face.m_verts[0]];
            const auto v1 = transformed[face.m_verts[1]];
            const auto v2 = transformed[face.m_verts[2]];

            const Triangle tri{ v0, v1, v2 };
            if ( dal::isIntersecting(seg, tri) ) {
                return true;
            }
        }

        return false;
    }

    std::optional<SegIntersecInfo> MeshData::findIntersection(const Segment& seg, const glm::mat4& transform) const {
        const auto transformed = this->makeTransformedVert(transform);

        std::optional<SegIntersecInfo> result{ std::nullopt };
        float dist = std::numeric_limits<float>::max();

        for ( const auto& face : this->m_faces ) {
            const auto v0 = transformed[face.m_verts[0]];
            const auto v1 = transformed[face.m_verts[1]];
            const auto v2 = transformed[face.m_verts[2]];

            const Triangle tri{ v0, v1, v2 };
            const auto col = dal::findIntersection(seg, tri);
            if ( col && col->m_distance < dist ) {
                dist = col->m_distance;
                result = col;
            }
        }

        return result;
    }

    // Private

    xvec3 MeshData::makeFaceNormal(const Face& face) const {
        auto& v0 = this->m_vertices[face.m_verts[0]];
        auto& v1 = this->m_vertices[face.m_verts[1]];
        auto& v2 = this->m_vertices[face.m_verts[2]];

        const glm::vec3 edge0 = v1 - v0;
        const glm::vec3 edge1 = v2 - v0;

        return glm::cross(edge0, edge1);
    }

    std::vector<glm::vec3> MeshData::makeTransformedVert(const glm::mat4& transform) const {
        std::vector<glm::vec3> transformed;
        transformed.reserve(this->m_vertices.size());
        for ( const auto& vert : this->m_vertices ) {
            const glm::vec3 tv = transform * glm::vec4{ vert, 1 };
            transformed.push_back(tv);
        }

        return transformed;
    }

}
