#include "u_objparser.h"

#include <fmt/format.h>

#include <daltools/model_parser.h>
#include <daltools/compression.h>

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


namespace {

    template <typename T>
    std::vector<T> reduce_joint_count_per_vertex(const std::vector<T>& src) {
        std::vector<T> output;

        dalAssert(0 == src.size() % 4);
        const auto vertex_count = src.size() / 4;
        output.reserve(vertex_count * 3);

        for (size_t i = 0; i < vertex_count; ++i) {
            auto& vec = *reinterpret_cast<const glm::tvec4<T>*>(src.data() + 4*i);
            output.push_back(vec[0]);
            output.push_back(vec[1]);
            output.push_back(vec[2]);
        }

        return output;
    }

    void convert_material(dal::binfo::Material& dst, const dal::parser::Material& src) {
        dst.m_roughness = src.m_roughness;
        dst.m_metallic = src.m_metallic;

        dst.m_diffuseMap = src.m_albedo_map;
        dst.m_roughnessMap = src.m_roughness_map;
        dst.m_metallicMap = src.m_metallic_map;
        dst.m_normalMap = src.m_normal_map;
    }

    void convert_model(dal::binfo::Model& dst, const dal::parser::Model& src) {
        // AABB
        {
            dst.m_aabb.set(
                src.m_aabb.m_min,
                src.m_aabb.m_max
            );
        }

        // Render units
        {
            for (auto& src_unit : src.m_units_straight) {
                auto& dst_unit = dst.m_renderUnits.emplace_back();

                dst_unit.m_name = src_unit.m_name;
                ::convert_material(dst_unit.m_material, src_unit.m_material);

                dst_unit.m_mesh.m_vertices = src_unit.m_mesh.m_vertices;
                dst_unit.m_mesh.m_normals = src_unit.m_mesh.m_normals;
                dst_unit.m_mesh.m_texcoords = src_unit.m_mesh.m_texcoords;
            }

            for (auto& src_unit : src.m_units_straight_joint) {
                auto& dst_unit = dst.m_renderUnits.emplace_back();

                dst_unit.m_name = src_unit.m_name;
                ::convert_material(dst_unit.m_material, src_unit.m_material);

                dst_unit.m_mesh.m_vertices = src_unit.m_mesh.m_vertices;
                dst_unit.m_mesh.m_normals = src_unit.m_mesh.m_normals;
                dst_unit.m_mesh.m_texcoords = src_unit.m_mesh.m_texcoords;
                dst_unit.m_mesh.m_boneWeights = src_unit.m_mesh.m_boneWeights;
                dst_unit.m_mesh.m_boneIndex = src_unit.m_mesh.m_boneIndex;
            }
        }

        // Skeleton
        {
            for (auto& src_joint : src.m_skeleton.m_joints) {
                const auto jid = dst.m_joints.getOrMakeIndexOf(src_joint.m_name);
                auto& dst_joint = dst.m_joints.at(jid);

                dst_joint.setName(src_joint.m_name);
                dst_joint.setOffset(src_joint.m_offset_mat);
                dst_joint.setParentIndex(src_joint.m_parent_index);
                dst_joint.setType(src_joint.m_joint_type);
            }
        }
    }

    void convert_animations(
        std::vector<dal::Animation>& dst,
        const std::vector<dal::parser::Animation>& src,
        const dal::SkeletonInterface& skeleton
    ) {
        for (auto& src_anim : src) {
            auto& dst_anim = dst.emplace_back(src_anim.m_name, src_anim.m_ticks_par_sec, src_anim.m_duration_tick);

            for (size_t i = 0; i < skeleton.getSize(); ++i) {
                auto& joint_info = skeleton.at(i);
                auto& src_joint_index = src_anim.find_by_name(joint_info.name());

                if (src_joint_index.has_value()) {
                    auto& src_joint = src_anim.m_joints[src_joint_index.value()];
                    auto& dst_joint = dst_anim.newJoint();
                    dst_joint.set(src_joint);
                }
                else {
                    auto& dst_joint = dst_anim.newJoint();
                    dst_joint.setName(joint_info.name());
                }
            }
        }
    }

}


namespace dal {

    bool loadDalModel_old(const char* const respath, ModelLoadInfo& info) {
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
            const auto decom_result = dal::decompress_zip(unzipped.data(), unzipped.size(), filebuf.data() + zippedBytesOffset, filebuf.size() - zippedBytesOffset);
            if (dal::CompressResult::success != decom_result.m_result) {
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

    bool loadDalModel(const char* const respath, ModelLoadInfo& info) {
        // Load file contents
        std::vector<uint8_t> filebuf;
        if ( !loadFileBuffer(respath, filebuf) ) {
            return false;
        }

        // Parse model
        {
            const auto parsed_model = dal::parser::parse_dmd(filebuf.data(), filebuf.size());
            if (!parsed_model.has_value())
                return false;

            ::convert_model(info.m_model, parsed_model.value());
            ::convert_animations(info.m_animations, parsed_model->m_animations, info.m_model.m_joints);
        }

        // Convert texture names into respath
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

        // Check joint count
        {
            dal::jointID_t max_joint_id = -1;

            for (auto& unit : info.m_model.m_renderUnits) {
                for (const auto jid : unit.m_mesh.m_boneIndex) {
                    if (jid > max_joint_id) {
                        max_joint_id = jid;
                    }
                }
            }

            dalAssert(max_joint_id + 1 == info.m_model.m_joints.getSize());
        }

        // Reduce joint count per vertex
        for (auto& unit : info.m_model.m_renderUnits) {
            unit.m_mesh.m_boneIndex = ::reduce_joint_count_per_vertex(unit.m_mesh.m_boneIndex);
            unit.m_mesh.m_boneWeights = ::reduce_joint_count_per_vertex(unit.m_mesh.m_boneWeights);
        }

        return true;
    }

}
