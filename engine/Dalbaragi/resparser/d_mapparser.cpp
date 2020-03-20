#include "d_mapparser.h"

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

    const uint8_t* parseVec3(glm::vec3& info, const uint8_t* begin, const uint8_t* const end) {
        float fbuf[3];
        begin = dal::assemble4BytesArray<float>(begin, fbuf, 3);
        info.x = fbuf[0];
        info.y = fbuf[1];
        info.z = fbuf[2];

        return begin;
    }

    const uint8_t* parseStr(std::string& info, const uint8_t* begin, const uint8_t* const end) {
        const auto charPtr = reinterpret_cast<const char*>(begin);
        info = charPtr;
        begin += (info.size() + 1);

        assertHeaderPtr(begin, end);
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
        begin = parseFloatList(info.m_vertices, begin, end); assert(begin < end);
        begin = parseFloatList(info.m_uvCoords, begin, end); assert(begin < end);
        begin = parseFloatList(info.m_normals, begin, end); assert(begin < end);
        return begin;
    }

    const uint8_t* parseMaterial(dal::v1::Material& info, const uint8_t* begin, const uint8_t* const end) {
        {
            float floatBuf[7];
            begin = dal::assemble4BytesArray<float>(begin, floatBuf, 4);

            info.m_roughness = floatBuf[0];
            info.m_metallic = floatBuf[1];

            info.m_texScale.x = floatBuf[2];
            info.m_texScale.y = floatBuf[3];
        }

        begin = parseStr(info.m_diffuseMap, begin, end);
        begin = parseStr(info.m_roughnessMap, begin, end);
        begin = parseStr(info.m_metallicMap, begin, end);

        return begin;
    }

    const uint8_t* parseRenderUnit(dal::v1::RenderUnit& info, const uint8_t* begin, const uint8_t* const end) {
        // Mesh variant
        throw 4;

        begin = parseMaterial(info.m_material, begin, end);

        return begin;
    }

    const uint8_t* parseTransform(dal::v1::Transform& info, const uint8_t* begin, const uint8_t* const end) {
        float floatBuf[8];
        begin = dal::assemble4BytesArray<float>(begin, floatBuf, 8);

        info.m_pos = glm::vec3{ floatBuf[0], floatBuf[1], floatBuf[2] };
        info.m_rot = glm::quat{ floatBuf[6], floatBuf[3], floatBuf[4], floatBuf[5] };
        info.m_scale = floatBuf[7];

        return begin;
    }

    const uint8_t* parseStaticActor(dal::v1::StaticActor& info, const uint8_t* begin, const uint8_t* const end) {
        begin = parseStr(info.m_name, begin, end);
        begin = parseTransform(info.m_trans, begin, end);
        return begin;
    }

}


namespace {

    struct Metadata {
        int32_t m_binVersion;
    };


    const uint8_t* parseMetadata(Metadata& info, const uint8_t* begin, const uint8_t* const end) {
        info.m_binVersion = dal::makeInt4(begin); begin += 4;

        return begin;
    }

    const uint8_t* parseModelEmbedded(dal::v1::ModelEmbedded& info, const uint8_t* begin, const uint8_t* const end) {
        // Name
        {
            begin = parseStr(info.m_name, begin, end);
        }

        // Render units
        {
            const auto numUnits = dal::makeInt4(begin); begin += 4;
            info.m_renderUnits.resize(numUnits);

            for ( auto& unit : info.m_renderUnits ) {
                begin = parseRenderUnit(unit, begin, end);
            }
        }

        // Static actors
        {
            const auto numActors = dal::makeInt4(begin); begin += 4;
            info.m_staticActors.resize(numActors);

            for ( auto& actor : info.m_staticActors ) {
                begin = parseStaticActor(actor, begin, end);
            }
        }

        // Flag detailed collider
        const auto flagDetailedCol = dal::makeBool1(begin); begin += 1;

        // Has rotating actor
        const auto hasRotatingActor = dal::makeBool1(begin); begin += 1;

        // Build mesh data
        {
            for ( auto& unit : info.m_renderUnits ) {
                throw 4;
            }
        }

        // Bounding volume
        {
            if ( 1 == info.m_renderUnits.size() ) {
                throw 4;
            }
            else {
                throw 4;
            }
        }

        if ( flagDetailedCol ) {
            if ( 1 == info.m_renderUnits.size() ) {
                throw 4;
            }
            else {
                throw 4;
            }
        }

        assertHeaderPtr(begin, end);
        return begin;
    }

    const uint8_t* parseModelImported(dal::v1::ModelImported& info, const uint8_t* begin, const uint8_t* const end) {
        begin = parseStr(info.m_resID, begin, end);

        // Static actors
        {
            const auto numActors = dal::makeInt4(begin); begin += 4;
            info.m_staticActors.resize(numActors);

            for ( auto& actor : info.m_staticActors ) {
                begin = parseStaticActor(actor, begin, end);
            }
        }

        // Flag detailed collider
        {
            info.m_detailedCollider = dal::makeBool1(begin); begin += 1;
        }

        assertHeaderPtr(begin, end);
        return begin;
    }

    const uint8_t* parseWaterPlane(dal::v1::WaterPlane& info, const uint8_t* begin, const uint8_t* const end) {
        float fbuf[12];
        begin = dal::assemble4BytesArray<float>(begin, fbuf, 12);

        info.m_pos.x = fbuf[0];
        info.m_pos.y = fbuf[1];
        info.m_pos.z = fbuf[2];

        info.m_width = fbuf[3];
        info.m_height = fbuf[4];
        info.m_flowSpeed = fbuf[5];
        info.m_waveStrength = fbuf[6];
        info.m_darkestDepth = fbuf[7];

        info.m_deepColor.x = fbuf[8];
        info.m_deepColor.y = fbuf[9];
        info.m_deepColor.z = fbuf[10];

        info.m_reflectance = fbuf[11];

        return begin;
    }

    const uint8_t* parsePlight(dal::v1::PointLight& info, const uint8_t* begin, const uint8_t* const end) {
        float fbuf[7];
        begin = dal::assemble4BytesArray<float>(begin, fbuf, 7);

        info.m_color.x = fbuf[0];
        info.m_color.y = fbuf[1];
        info.m_color.z = fbuf[2];

        info.m_pos.x = fbuf[3];
        info.m_pos.y = fbuf[4];
        info.m_pos.z = fbuf[5];

        info.m_maxDistance = fbuf[6];

        return begin;
    }

}


#include <iostream>
#include <nlohmann/json.hpp>

namespace dal {

    std::optional<v1::MapChunkInfo> parseDLB_v1(const uint8_t* const buf, const size_t bufSize) {
        const char* const magicBits = "dalmap";
        if ( 0 != std::memcmp(buf, magicBits, 6) ) {
            //dalError("Given datablock does not start with magic numbers.");
            return std::nullopt;
        }

        const auto [data, dataSize] = uncompressMap(buf + 6, bufSize);
        if ( nullptr == data ) {
            return std::nullopt;
        }

        v1::MapChunkInfo info;

        const uint8_t* header = data.get();
        const uint8_t* const end = header + dataSize;

        try {
            {
                Metadata metadata;
                header = parseMetadata(metadata, header, end);
            }

            {
                const auto numModelEmbedded = makeInt4(header); header += 4;
                info.m_embeddedModels.resize(numModelEmbedded);

                for ( auto& mdl : info.m_embeddedModels ) {
                    header = parseModelEmbedded(mdl, header, end);
                }
            }

            {
                const auto numModelImported = makeInt4(header); header += 4;
                info.m_importedModels.resize(numModelImported);

                for ( auto& mdl : info.m_importedModels ) {
                    header = parseModelImported(mdl, header, end);
                }
            }

            {
                const auto numWaterPlanes = makeInt4(header); header += 4;
                info.m_waterPlanes.resize(numWaterPlanes);

                for ( auto& water : info.m_waterPlanes ) {
                    header = parseWaterPlane(water, header, end);
                }
            }

            {
                const auto numPlights = makeInt4(header); header += 4;
                info.m_plights.resize(numPlights);

                for ( auto& light : info.m_plights ) {
                    header = parsePlight(light, header, end);
                }
            }
        }
        catch ( CorruptedBinary ) {
            // dalError("Failed to parse map, maybe it is corrupted.");
            return std::nullopt;
        }

        assert(header == end);

        // Postprocess
        {
            for ( auto& model : info.m_embeddedModels ) {
                for ( auto& unit : model.m_renderUnits ) {
                    const auto scale = unit.m_material.m_texScale;
                    auto& texcoords = unit.m_mesh.m_uvCoords;

                    const auto numTexcoords = texcoords.size() / 2;
                    for ( unsigned i = 0; i < numTexcoords; ++i ) {
                        texcoords[2 * i + 0] *= scale.x;
                        texcoords[2 * i + 1] *= scale.y;
                    }
                }
            }
        }

        return info;
    }

    void testJson(void) {
        using json = nlohmann::json;

        json j;

        j["list"] = { 2, 4, "32" };
        j["nothing"] = nullptr;
        j["level1"]["level2"] = 44.5;

        const auto str = j.dump();
        const auto re = json::parse(str);

        std::cout << re.dump(4) << std::endl;
    }

}
