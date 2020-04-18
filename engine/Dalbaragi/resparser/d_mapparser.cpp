#include "d_mapparser.h"

#include <array>
#include <memory>

#define ZLIB_WINAPI
#include <zlib.h>

#include <u_byteutils.h>


namespace {

    size_t unzip(uint8_t* const dst, const size_t dstSize, const uint8_t* const src, const size_t srcSize) {
        static_assert(sizeof(Bytef) == sizeof(uint8_t));

        uLongf decomBufSize = dstSize;

        const auto res = uncompress(dst, &decomBufSize, src, srcSize);
        switch ( res ) {

        case Z_OK:
            return decomBufSize;
        case Z_BUF_ERROR:
            // dalError("Zlib fail: buffer is not large enough");
            return 0;
        case Z_MEM_ERROR:
            // dalError("Zlib fail: Insufficient memory");
            return 0;
        case Z_DATA_ERROR:
            // dalError("Zlib fail: Corrupted data");
            return 0;
        default:
            // dalError(fmt::format("Zlib fail: Unknown reason ({})", res));
            return 0;

        }
    }

    // Returns nullptr containing unique_ptr and 0 on failure.
    std::pair<std::unique_ptr<uint8_t[]>, size_t> uncompressMap(const uint8_t* const buf, const size_t bufSize) {
        const auto allocatedSize = static_cast<size_t>(1.01 * dal::makeInt4(buf));  // Just to ensure that buffer never lacks.
        std::unique_ptr<uint8_t[]> decomBuf{ new uint8_t[allocatedSize] };
        const auto unzippedSize = unzip(decomBuf.get(), allocatedSize, buf + 4, bufSize - 4);

        if ( 0 != unzippedSize ) {
            return std::make_pair(std::move(decomBuf), static_cast<size_t>(unzippedSize));
        }
        else {
            return std::make_pair(nullptr, 0);
        }
    }


    class CorruptedBinary {  };

    inline void assertHeaderPtr(const uint8_t* const begin, const uint8_t* const end) {
        if ( begin > end ) {
            throw CorruptedBinary{};
        }
    }

    inline void pushBackVec3(std::vector<float>& c, const glm::vec3 v) {
        c.push_back(v.x);
        c.push_back(v.y);
        c.push_back(v.z);
    }

    inline void pushBackVec2(std::vector<float>& c, const glm::vec2 v) {
        c.push_back(v.x);
        c.push_back(v.y);
    }

    template <typename T>
    std::array<T, 6> triangulateRect(const T& p1, const T& p2, const T& p3, const T& p4) {
        return { p1, p2, p3, p1, p3, p4 };
    }

    glm::vec3 calcTriangleNormal(const glm::vec3 p1, const glm::vec3 p2, const glm::vec3 p3) {
        const auto edge1 = p3 - p2;
        const auto edge2 = p1 - p2;
        return glm::normalize(glm::cross(edge1, edge2));
    }

}


// Primitives
namespace {

    const uint8_t* parseVec3(glm::vec3& info, const uint8_t* begin) {
        float fbuf[3];
        begin = dal::assemble4BytesArray<float>(begin, fbuf, 3);
        info.x = fbuf[0];
        info.y = fbuf[1];
        info.z = fbuf[2];

        return begin;
    }

    const uint8_t* parseStr(std::string& info, const uint8_t* begin) {
        info = reinterpret_cast<const char*>(begin);
        begin += (info.size() + 1);
        return begin;
    }

    const uint8_t* parseFloatList(std::vector<float>& info, const uint8_t* begin, const uint8_t* const end) {
        const auto arrSize = dal::makeInt4(begin); begin += 4;
        info.resize(arrSize);
        begin = dal::assemble4BytesArray<float>(begin, info.data(), arrSize);

        assertHeaderPtr(begin, end);
        return begin;
    }

}


// Data blocks
namespace {

    const uint8_t* parseMesh(dal::v1::Mesh& info, const uint8_t* begin, const uint8_t* const end) {
        const auto num_verts = dal::makeInt4(begin); begin += 4;
        const auto num_verts_3 = static_cast<int64_t>(num_verts) * 3;
        const auto num_verts_2 = static_cast<int64_t>(num_verts) * 2;

        info.m_vertices.resize(num_verts_3);
        begin = dal::assemble4BytesArray<float>(begin, info.m_vertices.data(), num_verts_3);
        assertHeaderPtr(begin, end);

        info.m_uvcoords.resize(num_verts_2);
        begin = dal::assemble4BytesArray<float>(begin, info.m_uvcoords.data(), num_verts_2);
        assertHeaderPtr(begin, end);

        info.m_normals.resize(num_verts_3);
        begin = dal::assemble4BytesArray<float>(begin, info.m_normals.data(), num_verts_3);
        assertHeaderPtr(begin, end);

        return begin;
    }

    const uint8_t* parseMaterial(dal::v1::Material& info, const uint8_t* begin, const uint8_t* const end) {
        {
            float floatBuf[2];
            begin = dal::assemble4BytesArray<float>(begin, floatBuf, 2);

            info.m_roughness = floatBuf[0];
            info.m_metallic = floatBuf[1];
        }

        begin = parseStr(info.m_albedoMap, begin);
        begin = parseStr(info.m_roughnessMap, begin);
        begin = parseStr(info.m_metallicMap, begin);
        begin = parseStr(info.m_normalMap, begin);

        return begin;
    }

    const uint8_t* parseRenderUnit(dal::v1::RenderUnit& info, const uint8_t* begin, const uint8_t* const end) {
        begin = parseMaterial(info.m_material, begin, end);
        begin = parseMesh(info.m_mesh, begin, end);

        begin = parseVec3(info.m_aabb.m_min, begin);
        begin = parseVec3(info.m_aabb.m_max, begin);

        return begin;
    }

    const uint8_t* parseStaticActor(dal::v1::StaticActor& info, const uint8_t* begin, const uint8_t* const end) {
        // Name
        {
            info.m_name = reinterpret_cast<const char*>(begin);
            begin += info.m_name.size() + 1;
        }
        
        // Transform
        {
            constexpr int FBUF_SIZE = 8;
            float fbuf[FBUF_SIZE];
            begin = dal::assemble4BytesArray<float>(begin, fbuf, FBUF_SIZE);

            info.m_trans.m_pos = { fbuf[0], fbuf[1], fbuf[2] };
            info.m_trans.m_quat = { fbuf[3], fbuf[4], fbuf[5], fbuf[6] };
            info.m_trans.m_scale = fbuf[7];
        }

        return begin;
    }

}


namespace dal {

    std::optional<v1::MapChunk> parseMapChunk_v1(const uint8_t* const buf, const size_t bufSize) {
        const char* const magicBits = "dalchk";
        if ( 0 != std::memcmp(buf, magicBits, 6) ) {
            //dalError("Given datablock does not start with magic numbers.");
            return std::nullopt;
        }

        const auto [data, dataSize] = uncompressMap(buf + 6, bufSize);
        if ( nullptr == data ) {
            return std::nullopt;
        }

        v1::MapChunk info;

        const uint8_t* header = data.get();
        const uint8_t* const end = header + dataSize;

        try {
            {
                const auto num_units = dal::makeInt4(header); header += 4;
                info.m_renderUnits.resize(num_units);
                for ( int32_t i = 0; i < num_units; ++i ) {
                    header = parseRenderUnit(info.m_renderUnits[i], header, end);
                }
            }

            {
                const auto num_static_actors = dal::makeInt4(header); header += 4;
                info.m_staticActors.resize(num_static_actors);
                for ( int32_t i = 0; i < num_static_actors; ++i ) {
                    header = parseStaticActor(info.m_staticActors[i], header, end);
                    info.m_staticActors[i].m_modelIndex = dal::makeInt4(header); header += 4;
                }
            }
        }
        catch ( CorruptedBinary ) {
            // dalError("Failed to parse map, maybe it is corrupted.");
            return std::nullopt;
        }

        assert(header == end);

        return info;
    }

}
