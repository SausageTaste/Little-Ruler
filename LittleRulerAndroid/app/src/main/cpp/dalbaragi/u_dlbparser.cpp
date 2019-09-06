#include "u_dlbparser.h"

#include <string>
#include <cstring>
#include <exception>

#define ZLIB_WINAPI
#include <zlib.h>
#include <fmt/format.h>

#include "s_logger_god.h"


using namespace fmt::literals;


namespace {

    bool isBigEndian() {
        short int number = 0x1;
        char* numPtr = (char*)&number;
        return numPtr[0] != 1;
    }


    bool makeBool1(const uint8_t* begin) {
        return (*begin) != static_cast<uint8_t>(0);
    }

    int32_t makeInt2(const uint8_t* begin) {
        static_assert(1 == sizeof(uint8_t), "Size of uint8 is not 1 byte. WTF???");
        static_assert(4 == sizeof(float), "Size of float is not 4 bytes.");

        uint8_t buf[4];

        if ( isBigEndian() ) {
            buf[0] = 0;
            buf[1] = 0;
            buf[2] = begin[1];
            buf[3] = begin[0];
        }
        else {
            buf[0] = begin[0];
            buf[1] = begin[1];
            buf[2] = 0;
            buf[3] = 0;
        }

        int32_t res;
        memcpy(&res, buf, 4);
        return res;
    }

    int32_t makeInt4(const uint8_t* begin) {
        static_assert(1 == sizeof(uint8_t), "Size of uint8 is not 1 byte. WTF???");
        static_assert(4 == sizeof(float), "Size of float is not 4 bytes.");

        uint8_t buf[4];

        if ( isBigEndian() ) {
            buf[0] = begin[3];
            buf[1] = begin[2];
            buf[2] = begin[1];
            buf[3] = begin[0];
        }
        else {
            buf[0] = begin[0];
            buf[1] = begin[1];
            buf[2] = begin[2];
            buf[3] = begin[3];
        }

        int32_t res;
        memcpy(&res, buf, 4);
        return res;
    }

    float makeFloat4(const uint8_t* begin) {
        static_assert(1 == sizeof(uint8_t), "Size of uint8 is not 1 byte. WTF???");
        static_assert(4 == sizeof(float), "Size of float is not 4 bytes.");

        uint8_t buf[4];

        if ( isBigEndian() ) {
            buf[0] = begin[3];
            buf[1] = begin[2];
            buf[2] = begin[1];
            buf[3] = begin[0];
        }
        else {
            buf[0] = begin[0];
            buf[1] = begin[1];
            buf[2] = begin[2];
            buf[3] = begin[3];
        }

        float res;
        memcpy(&res, buf, 4);
        return res;
    }

    template <typename T>
    T make32Value(const uint8_t* const begin) {
        static_assert(1 == sizeof(uint8_t));
        static_assert(4 == sizeof(T));

        uint8_t buf[4];

        if ( isBigEndian() ) {
            buf[0] = begin[3];
            buf[1] = begin[2];
            buf[2] = begin[1];
            buf[3] = begin[0];
        }
        else {
            buf[0] = begin[0];
            buf[1] = begin[1];
            buf[2] = begin[2];
            buf[3] = begin[3];
        }

        T res;
        memcpy(&res, buf, 4);
        return res;
    }

    template <typename T>
    const uint8_t* make32ValueArr(const uint8_t* src, T* const dst, const size_t size) {
        for ( int i = 0; i < size; ++i ) {
            dst[i] = make32Value<T>(src); src += 4;
        }
        return src;
    }

    // Returns nullptr containing unique_ptr and 0 on failure.
    std::pair<std::unique_ptr<uint8_t[]>, size_t> uncompressMap(const uint8_t* const buf, const size_t bufSize) {
        const auto allocatedSize = makeInt4(buf) * 1.01;  // Just to ensure that buffer never lacks.
        std::unique_ptr<uint8_t[]> decomBuf{ new uint8_t[allocatedSize] };
        uLongf decomBufSize = allocatedSize;

        const auto res = uncompress(decomBuf.get(), &decomBufSize, buf + 4, bufSize);
        switch ( res ) {

        case Z_OK:
            return std::make_pair(std::move(decomBuf), static_cast<size_t>(decomBufSize));
        case Z_BUF_ERROR:
            dalError("Zlib fail: buffer is not large enough");
            return std::make_pair(nullptr, 0);
        case Z_MEM_ERROR:
            dalError("Zlib fail: Insufficient memory");
            return std::make_pair(nullptr, 0);
        case Z_DATA_ERROR:
            dalError("Zlib fail: Corrupted data");
            return std::make_pair(nullptr, 0);
        default:
            dalError("Zlib fail: Unknown reason ({})"_format(res));
            return std::make_pair(nullptr, 0);

        }
    }

    
    class CorruptedBinary {  };

    inline void assertHeaderPtr(const uint8_t* const begin, const uint8_t* const end) {
        if ( begin > end ) {
            throw CorruptedBinary{};
        }
    }

}


// Primitives
namespace {

    const uint8_t* parseVec3(glm::vec3& info, const uint8_t* begin, const uint8_t* const end) {
        float fbuf[3];
        begin = make32ValueArr(begin, fbuf, 3);
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
        const auto arrSize = makeInt4(begin); begin += 4;
        info.resize(arrSize);
        begin = make32ValueArr<float>(begin, info.data(), arrSize);

        assertHeaderPtr(begin, end);
        return begin;
    }

}


// Colliders
namespace {

    constexpr int COLLIDER_SPHERE = 1;
    constexpr int COLLIDER_AABB = 2;
    constexpr int COLLIDER_TRIANGLE = 3;
    constexpr int COLLIDER_TRIANGLE_SOUP = 4;


    const uint8_t* parseSphere(dal::Sphere& info, const uint8_t* begin, const uint8_t* const end) {
        float fbuf[4];
        begin = make32ValueArr(begin, fbuf, 4);

        info.setCenter(fbuf[0], fbuf[1], fbuf[2]);
        info.setRadius(fbuf[3]);

        return begin;
    }

    const uint8_t* parseAABB(dal::AABB& info, const uint8_t* begin, const uint8_t* const end) {
        glm::vec3 min, max;

        begin = parseVec3(min, begin, end);
        begin = parseVec3(max, begin, end);

        info.set(min, max);

        return begin;
    }

}


// Data blocks
namespace {

    const uint8_t* parseMesh(dal::dlb::RenderUnit::Mesh& info, const uint8_t* begin, const uint8_t* const end) {
        begin = parseFloatList(info.m_vertices, begin, end); dalAssert(begin < end);
        begin = parseFloatList(info.m_texcoords, begin, end); dalAssert(begin < end);
        begin = parseFloatList(info.m_normals, begin, end); dalAssert(begin < end);
        return begin;
    }

    const uint8_t* parseMaterial(dal::dlb::RenderUnit::Material& info, const uint8_t* begin, const uint8_t* const end) {
        {
            float floatBuf[7];
            begin = make32ValueArr<float>(begin, floatBuf, 7);

            info.m_baseColor.x = floatBuf[0];
            info.m_baseColor.y = floatBuf[1];
            info.m_baseColor.z = floatBuf[2];
            info.m_shininess = floatBuf[3];
            info.m_specStreng = floatBuf[4];
            info.m_texScale.x = floatBuf[5];
            info.m_texScale.y = floatBuf[6];
        }
        
        begin = parseStr(info.m_diffuseMap, begin, end);
        begin = parseStr(info.m_specularMap, begin, end);
        info.m_flagAlphaBlend = makeBool1(begin); begin += 1;

        return begin;
    }

    const uint8_t* parseRenderUnit(dal::dlb::RenderUnit& info, const uint8_t* begin, const uint8_t* const end) {
        begin = parseMesh(info.m_mesh, begin, end);
        begin = parseMaterial(info.m_material, begin, end);

        return begin;
    }

    const uint8_t* parseTransform(dal::Transform& info, const uint8_t* begin, const uint8_t* const end) {
        float floatBuf[8];
        begin = make32ValueArr(begin, floatBuf, 8);

        info.setPos(floatBuf[0], floatBuf[1], floatBuf[2]);
        info.setQuat(floatBuf[3], floatBuf[4], floatBuf[5], floatBuf[6]);
        info.setScale(floatBuf[7]);

        return begin;
    }

    const uint8_t* parseStaticActor(dal::ActorInfo& info, const uint8_t* begin, const uint8_t* const end) {
        begin = parseStr(info.m_name, begin, end);
        begin = parseTransform(info.m_transform, begin, end);
        return begin;
    }

}


namespace {

    struct Metadata {
        int32_t m_binVersion;
    };


    const uint8_t* parseMetadata(Metadata& info, const uint8_t* begin, const uint8_t* const end) {
        info.m_binVersion = makeInt4(begin); begin += 4;

        return begin;
    }

    const uint8_t* parseModelEmbedded(dal::dlb::ModelEmbedded& info, const uint8_t* begin, const uint8_t* const end) {
        begin = parseStr(info.m_name, begin, end);

        // Render units
        {
            const auto numUnits = makeInt4(begin); begin += 4;
            info.m_renderUnits.resize(numUnits);

            for ( auto& unit : info.m_renderUnits ) {
                begin = parseRenderUnit(unit, begin, end);
            }
        }

        // Static actors
        {
            const auto numActors = makeInt4(begin); begin += 4;
            info.m_staticActors.resize(numActors);

            for ( auto& actor : info.m_staticActors ) {
                begin = parseStaticActor(actor, begin, end);
            }
        }

        // Flag detailed collider
        if ( makeBool1(begin) ) {
            auto soup = new dal::ColTriangleSoup;
            info.m_detailed.reset(soup);

            for ( const auto& unit : info.m_renderUnits ) {
                auto& vertices = unit.m_mesh.m_vertices;

                const auto numTriangles = vertices.size() / 9;
                dalAssert((vertices.size() - 9 * numTriangles) == 0);

                for ( int i = 0; i < numTriangles; ++i ) {
                    const auto triIndex = 9 * i;
                    soup->addTriangle(dal::Triangle{
                        glm::vec3{ vertices[triIndex + 0], vertices[triIndex + 1], vertices[triIndex + 2] },
                        glm::vec3{ vertices[triIndex + 3], vertices[triIndex + 4], vertices[triIndex + 5] },
                        glm::vec3{ vertices[triIndex + 6], vertices[triIndex + 7], vertices[triIndex + 8] }
                        });
                }
            }
        } begin += 1;

        // Has rotating actor
        {
            const auto flag = makeBool1(begin); begin += 1;
        }

        // Bounding volume
        {
            const auto colCode = makeInt2(begin); begin += 2;
            switch ( colCode ) {

            case COLLIDER_SPHERE:
            {
                auto sphere = new dal::ColSphere;
                info.m_bounding.reset(sphere);
                begin = parseSphere(*sphere, begin, end);
                break;
            }
            case COLLIDER_AABB:
            {
                auto aabb = new dal::ColAABB;
                info.m_bounding.reset(aabb);
                begin = parseAABB(*aabb, begin, end);
                break;
            }
            default:
                dalAbort("Unkown collider type code: {}"_format(colCode));

            }
        }

        assertHeaderPtr(begin, end);
        return begin;
    }

    const uint8_t* parseModelImported(dal::dlb::ModelImported& info, const uint8_t* begin, const uint8_t* const end) {
        begin = parseStr(info.m_resourceID, begin, end);

        // Static actors
        {
            const auto numActors = makeInt4(begin); begin += 4;
            info.m_staticActors.resize(numActors);

            for ( auto& actor : info.m_staticActors ) {
                begin = parseStaticActor(actor, begin, end);
            }
        }

        // Flag detailed collider
        {
            info.m_detailedCollider = makeBool1(begin); begin += 1;
        }

        assertHeaderPtr(begin, end);
        return begin;
    }

    const uint8_t* parseWaterPlane(dal::dlb::WaterPlane& info, const uint8_t* begin, const uint8_t* const end) {
        float fbuf[14];
        begin = make32ValueArr<float>(begin, fbuf, 14);

        info.m_centerPos.x = fbuf[0];
        info.m_centerPos.y = fbuf[1];
        info.m_centerPos.z = fbuf[2];

        info.m_width = fbuf[3];
        info.m_height = fbuf[4];
        info.m_shininess = fbuf[5];
        info.m_specStreng = fbuf[6];
        info.m_flowSpeed = fbuf[7];
        info.m_waveStreng = fbuf[8];
        info.m_darkestDepth = fbuf[9];

        info.m_deepColor.x = fbuf[10];
        info.m_deepColor.y = fbuf[11];
        info.m_deepColor.z = fbuf[12];

        info.m_reflectivity = fbuf[13];

        return begin;
    }

}


namespace dal {

    std::optional<dlb::MapChunkInfo> parseDLB(const uint8_t* const buf, const size_t bufSize) {
        const auto [data, dataSize] = uncompressMap(buf, bufSize);
        if ( nullptr == data ) {
            return std::nullopt;
        }

        dlb::MapChunkInfo info;

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
        }
        catch ( CorruptedBinary ) {
            dalError("Failed to parse map, maybe it is corrupted.");
            return std::nullopt;
        }

        dalAssert(header == end);

        return info;
    }

}
