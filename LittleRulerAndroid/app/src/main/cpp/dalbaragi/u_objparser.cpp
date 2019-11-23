#include "u_objparser.h"

#include <unordered_map>

#include <glm/gtc/matrix_transform.hpp>
#include <fmt/format.h>

#include "s_logger_god.h"
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

    void makeAnimJoint_recur(dal::Animation::JointNode& parent, const dal::jointID_t parIndex, const dal::SkeletonInterface& skeleton, std::vector<dal::Animation::JointNode*>& resultList) {
        const auto numJoints = skeleton.getSize();

        size_t numChildren = 0;
        for ( int i = 0; i < numJoints; ++i ) {
            if ( skeleton.at(i).m_parentIndex == parIndex ) {
                numChildren++;
            }
        }
        parent.reserveChildrenVector(numChildren);

        for ( int i = 0; i < numJoints; ++i ) {
            if ( skeleton.at(i).m_parentIndex != parIndex ) {
                continue;
            }

            auto child = parent.emplaceChild(skeleton.getName(i), glm::mat4{ 1.f }, &parent);
            resultList[i] = child;
            makeAnimJoint_recur(*child, i, skeleton, resultList);
        }
    }

    auto makeAnimJointHierarchy(const dal::SkeletonInterface& skeleton) {
        std::pair<dal::Animation::JointNode, std::vector<dal::Animation::JointNode*>> result;

        const auto numJoints = skeleton.getSize();
        result.second.resize(numJoints);
        for ( auto& x : result.second ) {
            x = nullptr;
        }

        dalAssert(-1 == skeleton.at(0).m_parentIndex);

        result.first.setName(skeleton.getName(0));
        result.second[0] = &result.first;  // The address of root node changes upon return.

        makeAnimJoint_recur(result.first, 0, skeleton, result.second);

        for ( const auto x : result.second ) {
            dalAssert(nullptr != x);
        }

        return result;
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

            const auto result = skeleton.getOrMakeIndexOf(boneName);
            dalAssert(result == i);
            skeleton.at(result).m_parentIndex = parentIndex;
            header = parse_mat4(header, end, skeleton.at(result).m_boneOffset);
        }

        if ( skeleton.getSize() > 0 ) {
            skeleton.at(0).m_spaceToParent = skeleton.at(0).m_boneOffset;
            for ( int i = 1; i < numBones; ++i ) {
                const auto parentIndex = skeleton.at(i).m_parentIndex;
                const auto& parentOffset = skeleton.at(parentIndex).m_boneOffset;
                const auto& selfOffset = skeleton.at(i).m_boneOffset;
                skeleton.at(i).m_spaceToParent = glm::inverse(parentOffset) * selfOffset;
            }

            for ( int i = 0; i < numBones; ++i ) {
                skeleton.at(i).m_boneOffset = glm::inverse(skeleton.at(i).m_boneOffset);
            }
        }

        return header;
    }

    const uint8_t* parse_animJoint(const uint8_t* header, const uint8_t* const end, dal::Animation::JointNode& joint) {
        {
            glm::mat4 mat;
            header = parse_mat4(header, end, mat);
            joint.setMat(mat);
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
            auto [rootNode, jointList] = makeAnimJointHierarchy(skeleton);
            animations.emplace_back("nullptr", 0.f, 0.f, std::move(rootNode));
        }

        for ( int i = 0; i < numAnims; ++i ) {
            const std::string animName = reinterpret_cast<const char*>(header);
            header += animName.size() + 1;

            const auto durationTick = dal::assemble4Bytes<float>(header); header += 4;
            const auto tickPerSec = dal::assemble4Bytes<float>(header); header += 4;

            const auto numJoints = dal::makeInt4(header); header += 4;

            auto [rootNode, jointList] = makeAnimJointHierarchy(skeleton);
            jointList[0] = &rootNode;

            for ( int j = 0; j < numJoints; ++j ) {
                header = parse_animJoint(header, end, *jointList[j]);
            }
            jointList.clear();

            animations.emplace_back(animName, tickPerSec, durationTick, std::move(rootNode));
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
            const auto unzipSize = dal::unzip(unzipped.data(), unzipped.size(), filebuf.data() + zippedBytesOffset, filebuf.size() - zippedBytesOffset);
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
            }
        }

        return true;
    }

}
