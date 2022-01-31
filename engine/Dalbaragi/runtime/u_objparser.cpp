#include "u_objparser.h"

#include <daltools/model_parser.h>
#include <daltools/compression.h>

#include <d_logger.h>

#include "u_fileutils.h"


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

            if (dst.m_joints.getSize() > 0) {
                // Character lies on ground without this line.
                dst.m_joints.at(0).setParentMat(dst.m_joints.at(0).offset());

                for ( int i = 1; i < dst.m_joints.getSize(); ++i ) {
                    auto& this_info = dst.m_joints.at(i);
                    const auto& parent_info = dst.m_joints.at(this_info.parentIndex());
                    this_info.setParentMat(parent_info);
                }
            }
        }
    }

    void convert_animations(
        std::vector<dal::Animation>& dst,
        const std::vector<dal::parser::Animation>& src,
        const dal::SkeletonInterface& skeleton
    ) {
        // Insesrt null animation
        if (!src.empty()) {
            auto& anim = dst.emplace_back("nullpos", 0.f, 0.f);
            for ( int i = 0; i < skeleton.getSize(); ++i ) {
                anim.newJoint();
            }
        }

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
