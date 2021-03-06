#include "u_objparser.h"

#include <unordered_map>

#define ZLIB_WINAPI
#include <zlib.h>
#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>

#include <d_logger.h>

#include "u_timer.h"
#include "u_byteutils.h"
#include "u_fileutils.h"


using namespace fmt::literals;


// Consts
namespace {

    constexpr size_t MAGIC_NUMBER_COUNT = 6;
    constexpr char MAGIC_NUMBERS[] = "dalmdl";

}


// Utils
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


    bool checkMagicNumbers(const uint8_t* const begin) {
        for ( unsigned int i = 0; i < MAGIC_NUMBER_COUNT; ++i ) {
            if ( begin[i] != MAGIC_NUMBERS[i] ) {
                return false;
            }
        }

        return true;
    }

    const uint8_t* parse_aabb(const uint8_t* header, const uint8_t* const end, dal::AABB& aabb) {
        float fbuf[6];
        header = dal::assemble4BytesArray<float>(header, fbuf, 6);

        aabb.set(glm::vec3{ fbuf[0], fbuf[1], fbuf[2] }, glm::vec3{ fbuf[3], fbuf[4], fbuf[5] });

        return header;
    }

    const uint8_t* parse_mat4(const uint8_t* header, const uint8_t* const end, glm::mat4& mat) {
        float fbuf[16];
        header = dal::assemble4BytesArray<float>(header, fbuf, 16);

        for ( size_t row = 0; row < 4; ++row ) {
            for ( size_t col = 0; col < 4; ++col ) {
                mat[col][row] = fbuf[4 * row + col];
            }
        }

        return header;
    }

}


// Animation
namespace {

    const uint8_t* parse_skeleton(const uint8_t* header, const uint8_t* const end, dal::SkeletonInterface& skeleton) {
        const auto numBones = dal::makeInt4(header); header += 4;

        for ( int i = 0; i < numBones; ++i ) {
            const std::string boneName = reinterpret_cast<const char*>(header);
            header += boneName.size() + 1;
            const auto parentIndex = dal::makeInt4(header); header += 4;
            const auto boneTypeIndex = dal::makeInt4(header); header += 4;
            glm::mat4 offsetMat;
            header = parse_mat4(header, end, offsetMat);

            const auto jid = skeleton.getOrMakeIndexOf(boneName);
            dalAssert(jid == i);
            auto& jointInfo = skeleton.at(jid);

            jointInfo.setParentIndex(parentIndex);
            jointInfo.setOffset(offsetMat);

            switch ( boneTypeIndex ) {

            case 0:
                jointInfo.setType(dal::JointType::basic);
                break;
            case 1:
                jointInfo.setType(dal::JointType::hair_root);
                break;
            case 2:
                jointInfo.setType(dal::JointType::skirt_root);
                break;
            default:
                dalAbort(std::string{ "Unkown joint type index: " } +std::to_string(boneTypeIndex));

            }
        }

        if ( skeleton.getSize() > 0 ) {
            // Character lies on ground without this line.
            skeleton.at(0).setParentMat(skeleton.at(0).offset());

            for ( int i = 1; i < numBones; ++i ) {
                auto& thisInfo = skeleton.at(i);
                const auto& parentInfo = skeleton.at(thisInfo.parentIndex());
                thisInfo.setParentMat(parentInfo);
            }
        }

        return header;
    }

    const uint8_t* parse_animJoint(const uint8_t* header, const uint8_t* const end, dal::Animation::JointNode& joint) {
        {
            glm::mat4 mat;
            header = parse_mat4(header, end, mat);
        }

        {
            const auto num = dal::makeInt4(header); header += 4;
            for ( int i = 0; i < num; ++i ) {
                float fbuf[4];
                header = dal::assemble4BytesArray<float>(header, fbuf, 4);
                joint.addPos(fbuf[0], glm::vec3{ fbuf[1], fbuf[2], fbuf[3] });
            }
        }

        {
            const auto num = dal::makeInt4(header); header += 4;
            for ( int i = 0; i < num; ++i ) {
                float fbuf[5];
                header = dal::assemble4BytesArray<float>(header, fbuf, 5);
                joint.addRotation(fbuf[0], glm::quat{ fbuf[4], fbuf[1], fbuf[2], fbuf[3] });
            }
        }

        {
            const auto num = dal::makeInt4(header); header += 4;
            for ( int i = 0; i < num; ++i ) {
                float fbuf[2];
                header = dal::assemble4BytesArray<float>(header, fbuf, 2);
                joint.addScale(fbuf[0], fbuf[1]);
            }
        }

        return header;
    }

    const uint8_t* parse_animations(const uint8_t* header, const uint8_t* const end, const dal::SkeletonInterface& skeleton, std::vector<dal::Animation>& animations) {
        const auto numAnims = dal::makeInt4(header); header += 4;

        if ( numAnims > 0 ) {
            auto& anim = animations.emplace_back("nullpos", 0.f, 0.f);
            for ( int i = 0; i < skeleton.getSize(); ++i ) {
                anim.newJoint();
            }
        }

        for ( int i = 0; i < numAnims; ++i ) {
            const std::string animName = reinterpret_cast<const char*>(header);
            header += animName.size() + 1;

            const auto durationTick = dal::assemble4Bytes<float>(header); header += 4;
            const auto tickPerSec = dal::assemble4Bytes<float>(header); header += 4;

            auto& anim = animations.emplace_back(animName, tickPerSec, durationTick);

            const auto numJoints = dal::makeInt4(header); header += 4;

            for ( int j = 0; j < numJoints; ++j ) {
                header = parse_animJoint(header, end, anim.newJoint());
            }
        }

        return header;
    }

}


// DMD
namespace {

    const uint8_t* parse_material(const uint8_t* const begin, const uint8_t* const end, dal::binfo::Material& material) {
        const uint8_t* header = begin;

        material.m_roughness = dal::assemble4Bytes<float>(header); header += 4;
        material.m_metallic = dal::assemble4Bytes<float>(header); header += 4;

        material.m_diffuseMap = reinterpret_cast<const char*>(header);
        header += material.m_diffuseMap.size() + 1;

        material.m_roughnessMap = reinterpret_cast<const char*>(header);
        header += material.m_roughnessMap.size() + 1;

        material.m_metallicMap = reinterpret_cast<const char*>(header);
        header += material.m_metallicMap.size() + 1;

        material.m_normalMap = reinterpret_cast<const char*>(header);
        header += material.m_normalMap.size() + 1;

        return header;
    }

    const uint8_t* parse_renderUnit(const uint8_t* const begin, const uint8_t* const end, dal::binfo::RenderUnit& unit) {
        const uint8_t* header = begin;

        // Name
        {
            unit.m_name = reinterpret_cast<const char*>(header);
            header += unit.m_name.size() + 1;
        }

        // Material
        {
            header = parse_material(header, end, unit.m_material);
        }

        // Vertices
        {
            const auto numVert = dal::makeInt4(header); header += 4;
            const auto hasBones = dal::makeBool1(header); header += 1;
            const auto numVertTimes3 = numVert * 3;
            const auto numVertTimes2 = numVert * 2;

            unit.m_mesh.m_vertices.resize(numVertTimes3);
            header = dal::assemble4BytesArray<float>(header, unit.m_mesh.m_vertices.data(), numVertTimes3);

            unit.m_mesh.m_texcoords.resize(numVertTimes2);
            header = dal::assemble4BytesArray<float>(header, unit.m_mesh.m_texcoords.data(), numVertTimes2);

            unit.m_mesh.m_normals.resize(numVertTimes3);
            header = dal::assemble4BytesArray<float>(header, unit.m_mesh.m_normals.data(), numVertTimes3);

            if ( hasBones ) {
                unit.m_mesh.m_boneWeights.resize(numVertTimes3);
                header = dal::assemble4BytesArray<float>(header, unit.m_mesh.m_boneWeights.data(), numVertTimes3);

                unit.m_mesh.m_boneIndex.resize(numVertTimes3);
                header = dal::assemble4BytesArray<int32_t>(header, unit.m_mesh.m_boneIndex.data(), numVertTimes3);
            }
        }

        return header;
    }

}


namespace dal {

    bool loadDalModel(const char* const respath, ModelLoadInfo& info) {
        // Load file contents
        std::vector<uint8_t> filebuf;
        if ( !loadFileBuffer(respath, filebuf) ) {
            return false;
        }

        // Check magic numbers
        if ( !checkMagicNumbers(filebuf.data()) ) {
            return false;
        }

        // Decompress
        std::vector<uint8_t> unzipped;
        {
            const auto fullSize = dal::makeInt4(filebuf.data() + MAGIC_NUMBER_COUNT);
            const auto zippedBytesOffset = MAGIC_NUMBER_COUNT + 4;

            unzipped.resize(fullSize);
            const auto unzipSize = unzip(unzipped.data(), unzipped.size(), filebuf.data() + zippedBytesOffset, filebuf.size() - zippedBytesOffset);
            if ( 0 == unzipSize ) {
                return false;
            }
        }

        // Start parsing
        {
            const uint8_t* const end = unzipped.data() + unzipped.size();
            const uint8_t* header = unzipped.data();

            header = parse_aabb(header, end, info.m_model.m_aabb);
            header = parse_skeleton(header, end, info.m_model.m_joints);
            header = parse_animations(header, end, info.m_model.m_joints, info.m_animations);

            const auto numUnits = dal::makeInt4(header); header += 4;
            for ( int i = 0; i < numUnits; ++i ) {
                auto& unit = info.m_model.m_renderUnits.emplace_back();
                header = parse_renderUnit(header, end, unit);
            }

            dalAssert(header == end);
        }

        // Post process
        {
            for ( auto& unit : info.m_model.m_renderUnits ) {
                auto& mat = unit.m_material;

                if ( !mat.m_diffuseMap.empty() ) {
                    mat.m_diffuseMap = "::" + mat.m_diffuseMap;
                }
                if ( !mat.m_metallicMap.empty() ) {
                    mat.m_metallicMap = "::" + mat.m_metallicMap;
                }
                if ( !mat.m_roughnessMap.empty() ) {
                    mat.m_roughnessMap = "::" + mat.m_roughnessMap;
                }
                if ( !mat.m_normalMap.empty() ) {
                    mat.m_normalMap = "::" + mat.m_normalMap;
                }
            }

            // Apply texcoords resizing.
            for ( auto& unit : info.m_model.m_renderUnits ) {
                const auto scale = unit.m_material.m_texScale;
                auto& texcoords = unit.m_mesh.m_texcoords;

                const auto numTexcoords = texcoords.size() / 2;
                for ( unsigned i = 0; i < numTexcoords; ++i ) {
                    texcoords[2 * i + 0] *= scale.x;
                    texcoords[2 * i + 1] *= scale.y;
                }
            }
        }

        return true;
    }

}
