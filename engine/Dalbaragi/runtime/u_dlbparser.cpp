#include "u_dlbparser.h"

#include <string>
#include <cstring>
#include <exception>

#define ZLIB_WINAPI
#include <zlib.h>
#include <fmt/format.h>

#include <d_logger.h>
#include <u_byteutils.h>

#include "u_math.h"


using namespace fmt::literals;


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

    std::unique_ptr<dal::ICollider> buildAABB(const std::vector<float>& vertices) {
        dal::AABB aabb;

        const auto numVert = vertices.size() / 3;
        for ( size_t i = 0; i < numVert; ++i ) {
            const auto x = vertices[3 * i + 0];
            const auto y = vertices[3 * i + 1];
            const auto z = vertices[3 * i + 2];

            aabb.upscaleToInclude(x, y, z);
        }

        return std::unique_ptr<dal::ICollider>{ new dal::ColAABB{aabb} };
    }

    std::unique_ptr<dal::ICollider> buildSphere(const std::vector<float>& vertices) {
        dal::Sphere sphere;

        const auto numVert = vertices.size() / 3;
        for ( size_t i = 0; i < numVert; ++i ) {
            const auto x = vertices[3 * i + 0];
            const auto y = vertices[3 * i + 1];
            const auto z = vertices[3 * i + 2];

            sphere.upscaleToInclude(x, y, z);
        }

        return std::unique_ptr<dal::ICollider>{ new dal::ColSphere{ sphere } };
    }

    std::unique_ptr<dal::ICollider> buildTriSoup(const std::vector<float>& vertices) {
        std::unique_ptr<dal::ColTriangleSoup> soup{ new dal::ColTriangleSoup };

        const auto numTriangles = vertices.size() / 9;
        dalAssert((vertices.size() - 9 * numTriangles) == 0);

        for ( size_t i = 0; i < numTriangles; ++i ) {
            const auto triIndex = 9 * i;
            soup->addTriangle(dal::Triangle{
                glm::vec3{ vertices[triIndex + 0], vertices[triIndex + 1], vertices[triIndex + 2] },
                glm::vec3{ vertices[triIndex + 3], vertices[triIndex + 4], vertices[triIndex + 5] },
                glm::vec3{ vertices[triIndex + 6], vertices[triIndex + 7], vertices[triIndex + 8] }
                });
        }

        return soup;
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


// Colliders
namespace {

    constexpr int COLLIDER_SPHERE = 1;
    constexpr int COLLIDER_AABB = 2;
    constexpr int COLLIDER_TRIANGLE = 3;
    constexpr int COLLIDER_TRIANGLE_SOUP = 4;


    const uint8_t* parseSphere(dal::Sphere& info, const uint8_t* begin, const uint8_t* const end) {
        float fbuf[4];
        begin = dal::assemble4BytesArray<float>(begin, fbuf, 4);

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


// Mesh builders
namespace {
    namespace mesh {

        class FloatArray2D {

        private:
            std::vector<float> m_array;
            size_t m_rows = 0, m_columns = 0;

        public:
            const uint8_t* parse(const uint8_t* begin, const uint8_t* const end) {
                this->m_rows = dal::makeInt4(begin); begin += 4;
                this->m_columns = dal::makeInt4(begin); begin += 4;
                begin = parseFloatList(this->m_array, begin, end);
                return begin;
            }

            float getAt(const size_t row, const size_t column) const {
                dalAssert(this->isCoordInside(row, column));
                return this->m_array[this->calcTotalIndex(row, column)];
            }

            template <typename T>
            float getInterpAt(const T row, const T column) const {
                const glm::vec2 rowColVec{ static_cast<float>(row), static_cast<float>(column) };

                const auto floorRow = static_cast<size_t>(row);
                const auto floorRow_f = static_cast<float>(floorRow);
                const auto floorColumn = static_cast<size_t>(column);
                const auto floorColumn_f = static_cast<float>(floorColumn);

                // Order is floor, next row, next column, next both.
                

                const glm::ivec2 gridOffsets[4] = {
                    glm::ivec2{ 0, 0 },
                    glm::ivec2{ 1, 0 },
                    glm::ivec2{ 0, 1 },
                    glm::ivec2{ 1, 1 }
                };

                float values[4];
                float distSqrInv[4];

                for ( int i = 0; i < 4; ++i ) {
                    const auto rowGrid = static_cast<size_t>(row) + gridOffsets[i].x;
                    const auto colGrid = static_cast<size_t>(column) + gridOffsets[i].y;
                    if ( this->isCoordInside(rowGrid, colGrid) ) {
                        values[i] = this->getAt(rowGrid, colGrid);

                        const auto diffVec = rowColVec - glm::vec2{ static_cast<float>(rowGrid), static_cast<float>(colGrid) };
                        const auto distSqr = glm::dot(diffVec, diffVec);
                        if ( 0.f == distSqr ) {
                            return values[i];
                        }
                        else {
                            distSqrInv[i] = 1.f / distSqr;
                        }
                    }
                }

                const auto avrgValue = dal::calcWeightedAvrg(values, distSqrInv, 4);
                return avrgValue;
            }

            size_t getColumnSize(void) const {
                return this->m_columns;
            }

            size_t getRowSize(void) const {
                return this->m_rows;
            }

            bool isCoordInside(const size_t row, const size_t column) const {
                if ( this->m_rows <= row ) {
                    return false;
                }
                else if ( this->m_columns <= column ) {
                    return false;
                }
                else {
                    return true;
                }
            }

        private:
            size_t calcTotalIndex(const size_t row, const size_t column) const {
                return row * this->m_columns + column;
            }

        };


        class Rect : public dal::dlb::IMeshBuilder {

        private:
            glm::vec3 m_p00, m_p01, m_p10, m_p11;
            bool m_smoothShading = false;

        public:
            virtual const uint8_t* parse(const uint8_t* begin, const uint8_t* const end) override {
                // 4 vectors
                {
                    float fbuf[12];
                    begin = dal::assemble4BytesArray<float>(begin, fbuf, 12);

                    this->m_p00.x = fbuf[0];
                    this->m_p00.y = fbuf[1];
                    this->m_p00.z = fbuf[2];

                    this->m_p01.x = fbuf[3];
                    this->m_p01.y = fbuf[4];
                    this->m_p01.z = fbuf[5];

                    this->m_p10.x = fbuf[6];
                    this->m_p10.y = fbuf[7];
                    this->m_p10.z = fbuf[8];

                    this->m_p11.x = fbuf[9];
                    this->m_p11.y = fbuf[10];
                    this->m_p11.z = fbuf[11];
                }

                // Smooth shading
                {
                    this->m_smoothShading = dal::makeBool1(begin); begin += 1;
                }

                return begin;
            }

            virtual void makeVertexArray(std::vector<float>& vertices, std::vector<float>& texcoords, std::vector<float>& normals) const override {
                vertices.clear(); vertices.reserve(18);
                for ( const auto v : triangulateRect(this->m_p01, this->m_p00, this->m_p10, this->m_p11) ) {
                    pushBackVec3(vertices, v);
                }

                texcoords = {
                    0.f, 1.f,
                    0.f, 0.f,
                    1.f, 0.f,
                    0.f, 1.f,
                    1.f, 0.f,
                    1.f, 1.f
                };

                const auto a = this->m_p00 - this->m_p01;
                const auto b = this->m_p10 - this->m_p01;
                const auto c = this->m_p11 - this->m_p01;

                const auto normal1 = glm::normalize(glm::cross(a, b));
                const auto normal2 = glm::normalize(glm::cross(b, c));

                normals.clear(); normals.reserve(18);
                if ( this->m_smoothShading ) {
                    const auto normalAvrg = (normal1 + normal2) * 0.5f;
                    pushBackVec3(normals, normalAvrg);
                    pushBackVec3(normals, normal1);
                    pushBackVec3(normals, normalAvrg);
                    pushBackVec3(normals, normalAvrg);
                    pushBackVec3(normals, normalAvrg);
                    pushBackVec3(normals, normal2);
                }
                else {
                    pushBackVec3(normals, normal1);
                    pushBackVec3(normals, normal1);
                    pushBackVec3(normals, normal1);
                    pushBackVec3(normals, normal2);
                    pushBackVec3(normals, normal2);
                    pushBackVec3(normals, normal2);
                }
            }

        };


        class HeightGrid : public dal::dlb::IMeshBuilder {

        private:
            float m_xLen = 0.0f, m_zLen = 0.0f;
            FloatArray2D m_heightMap;
            bool m_smoothShading = false;

        public:
            virtual const uint8_t* parse(const uint8_t* begin, const uint8_t* const end) override {
                // xLen, zLen
                {
                    float fbuf[2];
                    begin = dal::assemble4BytesArray<float>(begin, fbuf, 2);

                    this->m_xLen = fbuf[0];
                    this->m_zLen = fbuf[1];
                }

                // Height map
                {
                    begin = this->m_heightMap.parse(begin, end);
                }

                // Smooth shading
                {
                    this->m_smoothShading = dal::makeBool1(begin); begin += 1;
                }

                return begin;
            }

            virtual void makeVertexArray(std::vector<float>& vertices, std::vector<float>& texcoords, std::vector<float>& normals) const override {
                const auto xGridSize = this->m_heightMap.getColumnSize();
                const auto zGridSize = this->m_heightMap.getRowSize();

                dal::Array2D<glm::vec3> pointsMap{ xGridSize, zGridSize };
                for ( size_t x = 0; x < xGridSize; ++x ) {
                    for ( size_t z = 0; z < zGridSize; ++z ) {
                        const auto point = this->makePointAt(x, z);
                        pointsMap.at(x, z) = point;
                    }
                }

                dal::Array2D<glm::vec2> texcoordsMap{ xGridSize, zGridSize };
                for ( size_t x = 0; x < xGridSize; ++x ) {
                    for ( size_t z = 0; z < zGridSize; ++z ) {
                        texcoordsMap.at(x, z) = this->makeTexcoordsAt(x, z);
                    }
                }

                dal::Array2D<glm::vec3> normalsMap{ xGridSize, zGridSize };
                for ( size_t x = 0; x < xGridSize; ++x ) {
                    for ( size_t z = 0; z < zGridSize; ++z ) {
                        normalsMap.at(x, z) = this->makePointNormalFor(x, z);
                    }
                }

                vertices.clear();
                texcoords.clear();
                normals.clear();

                for ( size_t x = 0; x < xGridSize - 1; ++x ) {
                    for ( size_t z = 0; z < zGridSize - 1; ++z ) {
                        const auto triangulatedIndices = triangulateRect(
                            std::pair{ x, z },
                            std::pair{ x, z + 1 },
                            std::pair{ x + 1, z + 1 },
                            std::pair{ x + 1, z }
                        );

                        std::vector<glm::vec3> vertexCache;
                        for ( const auto [xIndex, zIndex] : triangulatedIndices ) {
                            vertexCache.push_back(pointsMap.at(xIndex, zIndex));
                        }

                        const auto triNormal1 = calcTriangleNormal(vertexCache[0], vertexCache[1], vertexCache[2]);
                        const auto triNormal2 = calcTriangleNormal(vertexCache[3], vertexCache[4], vertexCache[5]);

                        for ( size_t i = 0; i < triangulatedIndices.size(); ++i ) {
                            const auto [xIndex, zIndex] = triangulatedIndices[i];

                            pushBackVec3(vertices, vertexCache[i]);
                            pushBackVec2(texcoords, texcoordsMap.at(xIndex, zIndex));

                            if ( this->m_smoothShading ) {
                                pushBackVec3(normals, normalsMap.at(xIndex, zIndex));
                            }
                            else {
                                const auto thisNormal = i < 3 ? triNormal1 : triNormal2;
                                pushBackVec3(normals, thisNormal);
                            }
                        }
                    }
                }
            }

        private:
            // Returns vec2{ x, z }
            template <typename T>
            glm::vec2 calcWorldPos(const T xGrid, const T zGrid) const {
                /*
                const auto numGridX = this->m_heightMap.getColumnSize();
                const auto numGridZ = this->m_heightMap.getRowSize();

                const auto xLeftInGlobal = -(this->m_xLen * 0.5f);
                const auto zFarInGlobal = -(this->m_zLen * 0.5f);

                const auto x = xLeftInGlobal + (this->m_xLen / float(numGridX - 1)) * float(xGrid);
                const auto z = zFarInGlobal + (this->m_zLen / float(numGridZ - 1)) * float(zGrid);

                Bellow is simplified of above.
                */

                glm::vec2 result;

                {
                    const auto numGridX = this->m_heightMap.getColumnSize();
                    const auto a = static_cast<float>(xGrid) / static_cast<float>(numGridX - 1);
                    result.x = this->m_xLen * (a - 0.5f);
                }
                
                {
                    const auto numGridZ = this->m_heightMap.getRowSize();
                    const auto a = static_cast<float>(zGrid) / static_cast<float>(numGridZ - 1);
                    result.y = this->m_zLen * (a - 0.5f);
                }

                return result;
            }

            glm::vec3 makePointAt(const size_t xGrid, const size_t zGrid) const {
                const auto worldPosXZ = calcWorldPos(xGrid, zGrid);

                const glm::vec3 result{
                    worldPosXZ.x,
                    this->m_heightMap.getAt(zGrid, xGrid),
                    worldPosXZ.y
                };

                return result;
            }

            glm::vec3 makePointInterp(const float xGrid, const float zGrid) const {
                const auto worldPosXZ = calcWorldPos(xGrid, zGrid);
                const auto height = this->m_heightMap.getInterpAt<float>(xGrid, zGrid);
                return glm::vec3{ worldPosXZ.x, height, worldPosXZ.y };
            }

            glm::vec2 makeTexcoordsAt(const size_t xGrid, const size_t zGrid) const {
                const auto xGridCount = this->m_heightMap.getColumnSize();
                const auto zGridCount = this->m_heightMap.getRowSize();
                return glm::vec2(float(xGrid) / float(xGridCount - 1), float(zGrid) / float(zGridCount - 1));
            }

            glm::vec3 makePointNormalFor(const size_t xGrid, const size_t zGrid) const {
                const auto inputPoint = this->makePointAt(xGrid, zGrid);

                std::array<glm::ivec2, 4> adjacentOffsets = {
                    glm::ivec2{0, -1},
                    glm::ivec2{-1, 0},
                    glm::ivec2{0, 1},
                    glm::ivec2{1, 0}
                };
                std::array<std::optional<glm::vec3>, 4> adjacentPoints;

                for ( size_t i = 0; i < adjacentOffsets.size(); ++i ) {
                    const auto offset = adjacentOffsets[i];
                    if ( this->m_heightMap.isCoordInside(xGrid + offset[0], zGrid + offset[1]) ) {
                        adjacentPoints[i] = this->makePointAt(xGrid + offset[0], zGrid + offset[1]);
                    }
                    else {
                        adjacentPoints[i] = std::nullopt;
                    }
                }

                glm::vec3 normalAccum;
                size_t addedCount = 0;

                for ( int i = 0; i < 4; ++i ) {
                    const auto toEdge1 = i;
                    const auto toEdge2 = (i + 1) % 4;

                    if ( (!adjacentPoints[toEdge1]) || (!adjacentPoints[toEdge2]) ) {
                        continue;
                    }

                    const auto edge1 = *adjacentPoints[toEdge1] - inputPoint;
                    const auto edge2 = *adjacentPoints[toEdge2] - inputPoint;

                    normalAccum += glm::normalize(glm::cross(edge1, edge2));
                    addedCount += 1;
                }

                dalAssert(0 != addedCount);
                return glm::normalize(normalAccum / static_cast<float>(addedCount));
            }

        };

    }
}


// Data blocks
namespace {

    const uint8_t* parseMesh(dal::binfo::Mesh& info, const uint8_t* begin, const uint8_t* const end) {
        begin = parseFloatList(info.m_vertices, begin, end); dalAssert(begin < end);
        begin = parseFloatList(info.m_texcoords, begin, end); dalAssert(begin < end);
        begin = parseFloatList(info.m_normals, begin, end); dalAssert(begin < end);
        return begin;
    }

    const uint8_t* parseMaterial(dal::binfo::Material& info, const uint8_t* begin, const uint8_t* const end) {
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

    const uint8_t* parseRenderUnit(dal::dlb::RenderUnit& info, const uint8_t* begin, const uint8_t* const end) {
        // Mesh variant
        {
            const auto typeCode = dal::makeInt2(begin); begin += 2;
            switch ( typeCode ) {

            case 0:
                info.m_meshBuilder.reset(new mesh::Rect);
                begin = info.m_meshBuilder->parse(begin, end);
                break;
            case 1:
                info.m_meshBuilder.reset(new mesh::HeightGrid);
                begin = info.m_meshBuilder->parse(begin, end);
                break;
            default:
                dalAbort("Unknown type code for mesh builder: {}"_format(typeCode));

            }
        }

        begin = parseMaterial(info.m_material, begin, end);

        return begin;
    }

    const uint8_t* parseTransform(dal::Transform& info, const uint8_t* begin, const uint8_t* const end) {
        float floatBuf[8];
        begin = dal::assemble4BytesArray<float>(begin, floatBuf, 8);

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
        info.m_binVersion = dal::makeInt4(begin); begin += 4;

        return begin;
    }

    const uint8_t* parseModelEmbedded(dal::dlb::ModelEmbedded& info, const uint8_t* begin, const uint8_t* const end) {
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
                unit.m_meshBuilder->makeVertexArray(unit.m_mesh.m_vertices, unit.m_mesh.m_texcoords, unit.m_mesh.m_normals);
            }
        }

        // Bounding volume
        {
            if ( 1 == info.m_renderUnits.size() ) {
                info.m_bounding = hasRotatingActor ? buildSphere(info.m_renderUnits[0].m_mesh.m_vertices) : buildAABB(info.m_renderUnits[0].m_mesh.m_vertices);
            }
            else {
                dalAbort("Not implemented")
            }
        }

        if ( flagDetailedCol ) {
            if ( 1 == info.m_renderUnits.size() ) {
                info.m_detailed = buildTriSoup(info.m_renderUnits[0].m_mesh.m_vertices);
            }
            else {
                dalAbort("Not implemented")
            }
        }

        assertHeaderPtr(begin, end);
        return begin;
    }

    const uint8_t* parseModelImported(dal::dlb::ModelImported& info, const uint8_t* begin, const uint8_t* const end) {
        begin = parseStr(info.m_resourceID, begin, end);

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

    const uint8_t* parseWaterPlane(dal::dlb::WaterPlane& info, const uint8_t* begin, const uint8_t* const end) {
        float fbuf[12];
        begin = dal::assemble4BytesArray<float>(begin, fbuf, 12);

        info.m_centerPos.x = fbuf[0];
        info.m_centerPos.y = fbuf[1];
        info.m_centerPos.z = fbuf[2];

        info.m_width = fbuf[3];
        info.m_height = fbuf[4];
        info.m_flowSpeed = fbuf[5];
        info.m_waveStreng = fbuf[6];
        info.m_darkestDepth = fbuf[7];

        info.m_deepColor.x = fbuf[8];
        info.m_deepColor.y = fbuf[9];
        info.m_deepColor.z = fbuf[10];

        info.m_reflectivity = fbuf[11];

        return begin;
    }

    const uint8_t* parsePlight(dal::dlb::Plight& info, const uint8_t* begin, const uint8_t* const end) {
        float fbuf[7];
        begin = dal::assemble4BytesArray<float>(begin, fbuf, 7);

        info.m_color.x = fbuf[0];
        info.m_color.y = fbuf[1];
        info.m_color.z = fbuf[2];

        info.m_pos.x = fbuf[3];
        info.m_pos.y = fbuf[4];
        info.m_pos.z = fbuf[5];

        info.m_maxDist = fbuf[6];

        return begin;
    }

}


namespace dal {

    std::optional<dlb::MapChunkInfo> parseDLB(const uint8_t* const buf, const size_t bufSize) {
        const char* const magicBits = "dalmap";
        if ( 0 != std::memcmp(buf, magicBits, 6) ) {
            dalError("Given datablock does not start with magic numbers.");
            return std::nullopt;
        }

        const auto [data, dataSize] = uncompressMap(buf + 6, bufSize);
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

            {
                const auto numPlights = makeInt4(header); header += 4;
                info.m_plights.resize(numPlights);

                for ( auto& light : info.m_plights ) {
                    header = parsePlight(light, header, end);
                }
            }
        }
        catch ( CorruptedBinary ) {
            dalError("Failed to parse map, maybe it is corrupted.");
            return std::nullopt;
        }

        dalAssert(header == end);

        // Postprocess
        {
            for ( auto& model : info.m_embeddedModels ) {
                for ( auto& unit : model.m_renderUnits ) {
                    const auto scale = unit.m_material.m_texScale;
                    auto& texcoords = unit.m_mesh.m_texcoords;

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

}
