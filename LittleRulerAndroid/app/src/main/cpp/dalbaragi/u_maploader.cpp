#include "u_maploader.h"

#include <cstring>
#include <string>

#include <fmt/format.h>
#define ZLIB_WINAPI
#include <zlib.h>

#include "s_logger_god.h"


using namespace fmt::literals;


namespace {
    namespace typeCodes {

        constexpr int attrib_actor = 3;

        constexpr int item_modelImported = 2;
        constexpr int item_modelDefined = 1;
        constexpr int item_modelImportedAnimated = 6;

        constexpr int item_lightPoint = 4;

        constexpr int item_waterPlane = 5;

    }
}


namespace {

    bool isBigEndian() {
        short int number = 0x1;
        char* numPtr = (char*)& number;
        return numPtr[0] != 1;
    }


    int makeInt2(const uint8_t* begin) {
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

        int res;
        memcpy(&res, buf, 4);
        return res;
    }

    int makeInt4(const uint8_t* begin) {
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

        int res;
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

}


namespace {  // Make attribs

    const uint8_t* makeAttrib_actor(std::vector<dal::ActorInfo>& actorVec, const uint8_t* const begin, const uint8_t* const end) {
        const uint8_t* header = begin;

        // Construct
        {
            const auto charPtr = reinterpret_cast<const char*>(begin);
            const auto len = std::strlen(charPtr);
            if ( len > 512 ) dalAbort("Length of string is bigger than 512.");
            header += len + 1;

            // Static flag
            const auto flagStatic = 0 != *header++;

            //

            actorVec.emplace_back(charPtr, flagStatic);
        }

        auto& actor = actorVec.back();

        {
            const size_t assumedRestBytes = 4 * (4 + 3);  // (float is 4 bytes) * ( (vec4) + (vec3) )
            if ( assumedRestBytes > (end - header) ) {
                dalAbort("Umm.. what was this??");
            }
        }

        {
            // vec3 as pos, vec4 as quat, float as scale
            constexpr auto readSize = 8;
            float numBuf[readSize];
            for ( unsigned int i = 0; i < readSize; i++ ) {
                numBuf[i] = makeFloat4(header);
                header += 4;
            }

            actor.setPos(glm::vec3{ numBuf[0], numBuf[1], numBuf[2] });
            actor.setQuat(glm::quat{ numBuf[6], numBuf[3], numBuf[4], numBuf[5] });
            actor.setScale(numBuf[7]);
        }

        return header;
    }

}


namespace {  // Make items

    const uint8_t* make_modelImported(dal::loadedinfo::LoadedMap& info, const uint8_t* const begin, const uint8_t* const end) {
        const uint8_t* header = begin;
        info.m_importedModels.emplace_back();
        auto& importedModel = info.m_importedModels.back();

        { // Get model id
            const auto charPtr = reinterpret_cast<const char*>(begin);
            importedModel.m_modelID = charPtr;
            header += std::strlen(charPtr) + 1;
        }

        { // Get actors
            const auto listElementTypeCode = makeInt2(header);
            assert(typeCodes::attrib_actor == listElementTypeCode);
            header += 2;
            const auto listSize = makeInt4(header);
            header += 4;

            for ( int i = 0; i < listSize; i++ ) {
                header = makeAttrib_actor(importedModel.m_actors, header, end);
            }
        }

        return header;
    }

    const uint8_t* make_modelDefined(dal::loadedinfo::LoadedMap& info, const uint8_t* const begin, const uint8_t* const end) {
        const uint8_t* header = begin;
        info.m_definedModels.emplace_back();
        auto& definedModel = info.m_definedModels.back();

        // Parse model id
        {
            const auto charPtr = reinterpret_cast<const char*>(header);
            definedModel.m_modelID = charPtr;
            header += std::strlen(charPtr) + 1;
        }

        // Parse actors
        {
            const auto listElementTypeCode = makeInt2(header); header += 2;
            assert(typeCodes::attrib_actor == listElementTypeCode);

            const auto listSize = makeInt4(header); header += 4;


            for ( int i = 0; i < listSize; i++ ) {
                header = makeAttrib_actor(definedModel.m_actors, header, end);
            }
        }

        // Parse vertex arrays
        {
            std::vector<float>* arrays[3] = {
                &definedModel.m_renderUnit.m_mesh.m_vertices,
                &definedModel.m_renderUnit.m_mesh.m_texcoords,
                &definedModel.m_renderUnit.m_mesh.m_normals
            };

            for ( int i = 0; i < 3; i++ ) {
                const auto arrSize = makeInt4(header); header += 4;

                auto arr = arrays[i];
                arr->resize(arrSize);

                for ( int j = 0; j < arrSize; j++ ) {
                    const auto value = makeFloat4(header); header += 4;
                    arr->at(j) = value;
                }
            }
        }

        // Make bounding box
        {
            assert(0 == (definedModel.m_renderUnit.m_mesh.m_vertices.size() % 3));
            auto iter = definedModel.m_renderUnit.m_mesh.m_vertices.begin();

            glm::vec3 aabbP1, aabbP2;
            while ( definedModel.m_renderUnit.m_mesh.m_vertices.end() != iter ) {
                const auto x = *iter++;
                const auto y = *iter++;
                const auto z = *iter++;

                if ( aabbP1.x > x )
                    aabbP1.x = x;
                else if ( aabbP2.x < x )
                    aabbP2.x = x;

                if ( aabbP1.y > y )
                    aabbP1.y = y;
                else if ( aabbP2.y < y )
                    aabbP2.y = y;

                if ( aabbP1.z > z )
                    aabbP1.z = z;
                else if ( aabbP2.z < z )
                    aabbP2.z = z;
            }
            definedModel.m_boundingBox.set(aabbP1, aabbP2);
        }

        auto& material = definedModel.m_renderUnit.m_material;

        // Parse diffuse color(3), shininess(1), specular strength(1), texSize(1, 1)
        {
            float floatBuf[7];
            for ( int i = 0; i < 7; i++ ) {
                floatBuf[i] = makeFloat4(header); header += 4;
            }

            material.m_diffuseColor = { floatBuf[0], floatBuf[1], floatBuf[2] };
            material.m_shininess = floatBuf[3];
            material.m_specStrength = floatBuf[4];
            material.m_texSize.x = floatBuf[5];
            material.m_texSize.y = floatBuf[6];
        }

        // Parse diffuse map name
        {
            const auto charPtr = reinterpret_cast<const char*>(header);
            material.m_diffuseMap = charPtr;
            header += std::strlen(charPtr) + 1;
        }

        // Parse specular map name
        {
            const auto charPtr = reinterpret_cast<const char*>(header);
            material.m_specularMap = charPtr;
            header += std::strlen(charPtr) + 1;
        }

        return header;
    }

    const uint8_t* make_modelImportedAnimated(dal::loadedinfo::LoadedMap& info, const uint8_t* const begin, const uint8_t* const end) {
        const uint8_t* header = begin;
        info.m_animatedModels.emplace_back();
        auto& animatedModel = info.m_animatedModels.back();

        { // Get model id
            const auto charPtr = reinterpret_cast<const char*>(begin);
            animatedModel.m_modelID = charPtr;
            header += std::strlen(charPtr) + 1;
        }

        { // Get actors
            const auto listElementTypeCode = makeInt2(header);
            assert(typeCodes::attrib_actor == listElementTypeCode);
            header += 2;
            const auto listSize = makeInt4(header);
            header += 4;

            for ( int i = 0; i < listSize; i++ ) {
                header = makeAttrib_actor(animatedModel.m_actors, header, end);
            }
        }

        return header;
    }


    const uint8_t* make_lightPoint(dal::loadedinfo::LoadedMap& info, const uint8_t* const begin, const uint8_t* const end) {
        const uint8_t* header = begin;
        info.m_pointLights.emplace_back();
        auto& plight = info.m_pointLights.back();

        // str light_name
        {
            const auto charPtr = reinterpret_cast<const char*>(header);
            plight.m_name = charPtr;
            header += std::strlen(charPtr) + 1;
        }

        // Static
        {
            const auto flag = *header;
            if ( 0 == flag ) plight.m_static = false;
            else plight.m_static = true;
            header++;
        }

        // vec3 pos, color; float max_dist
        {
            float floatBuf[7];
            for ( int i = 0; i < 7; i++ ) {
                floatBuf[i] = makeFloat4(header); header += 4;
            }

            plight.m_color = { floatBuf[0], floatBuf[1], floatBuf[2] };
            plight.m_pos = { floatBuf[3], floatBuf[4], floatBuf[5] };
            plight.m_maxDist = floatBuf[6];
        }

        return header;
    }


    const uint8_t* make_waterPlane(dal::loadedinfo::LoadedMap& info, const uint8_t* const begin, const uint8_t* const end) {
        auto header = begin;
        info.m_waterPlanes.emplace_back();
        auto& water = info.m_waterPlanes.back();

        // Get diffuse pos(3), width(1), height(1), shininess(1), specStreng(1),
        // move speed(1), wave streng(1), darkest depth point(1), depth color(3), reflectivity(1)
        {
            constexpr auto k_numNums = 14;
            float floatBuf[k_numNums];
            for ( int i = 0; i < k_numNums; i++ ) {
                floatBuf[i] = makeFloat4(header); header += 4;
            }

            water.m_pos = { floatBuf[0], floatBuf[1], floatBuf[2] };
            water.m_width = floatBuf[3];
            water.m_height = floatBuf[4];
            water.m_shineness = floatBuf[5];
            water.m_specStreng = floatBuf[6];
            water.m_moveSpeed = floatBuf[7];
            water.m_waveStreng = floatBuf[8];
            water.m_darkestDepthPoint = floatBuf[9];
            water.m_depthColor = glm::vec3{ floatBuf[10], floatBuf[11], floatBuf[12] };
            water.m_reflectivity = floatBuf[13];
        }

        return header;
    }


    decltype(make_modelImported)* selectMakerFunc(const int typeCode) {

        switch ( typeCode ) {

        case typeCodes::item_modelDefined:
            return make_modelDefined;
        case typeCodes::item_modelImported:
            return make_modelImported;
        case typeCodes::item_modelImportedAnimated:
            return make_modelImportedAnimated;

        case typeCodes::item_lightPoint:
            return make_lightPoint;

        case typeCodes::item_waterPlane:
            return make_waterPlane;

        default:
            dalError("Unknown map item typeCode: {}"_format(typeCode));
            return nullptr;

        }

    }

}


namespace dal {

    bool parseMap_dlb(loadedinfo::LoadedMap& info, const uint8_t* const buf, const size_t bufSize) {
        const auto chunkSize = bufSize * 20;
        std::unique_ptr<uint8_t[]> decomBuf{ new uint8_t[chunkSize] };
        uLongf decomBufSize = chunkSize;

        {
            const auto res = uncompress(decomBuf.get(), &decomBufSize, buf, bufSize);
            switch ( res ) {

            case Z_OK:
                break;
            case Z_BUF_ERROR:
                dalError("Zlib fail: buffer is not large enough");
                return false;
            case Z_MEM_ERROR:
                dalError("Zlib fail: Insufficient memory");
                return false;
            case Z_DATA_ERROR:
                dalError("Zlib fail: Corrupted data");
                return false;
            default:
                dalError("Zlib fail: Unknown reason ({})"_format(res));
                return false;

            }
        }

        const auto end = decomBuf.get() + decomBufSize;
        const uint8_t* header = decomBuf.get();

        while ( true ) {
            const auto typeCode = makeInt2(header);
            auto makerFunc = selectMakerFunc(typeCode);
            if ( nullptr == makerFunc )
                return false;

            header += 2;

            header = makerFunc(info, header, end);
            if ( header == end )
                return true;
        }
    }

}