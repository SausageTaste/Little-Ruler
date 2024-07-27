#include "u_objparser.h"

#include <daltools/dmd/parser.h>
#include <daltools/common/compression.h>

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
        dst.m_roughness = src.roughness_;
        dst.m_metallic = src.metallic_;

        dst.m_diffuseMap = src.albedo_map_;
        dst.m_roughnessMap = src.roughness_map_;
        dst.m_metallicMap = src.metallic_map_;
        dst.m_normalMap = src.normal_map_;
    }

    void convert_model(dal::binfo::Model& dst, const dal::parser::Model& src) {
        // AABB
        {
            dst.m_aabb.set(
                src.aabb_.min_,
                src.aabb_.max_
            );
        }

        // Render units
        {
            for (auto& src_unit : src.units_straight_) {
                auto& dst_unit = dst.m_renderUnits.emplace_back();

                dst_unit.m_name = src_unit.name_;
                ::convert_material(dst_unit.m_material, src_unit.material_);

                dst_unit.m_mesh.m_vertices = src_unit.mesh_.vertices_;
                dst_unit.m_mesh.m_normals = src_unit.mesh_.normals_;
                dst_unit.m_mesh.m_texcoords = src_unit.mesh_.uv_coordinates_;
            }

            for (auto& src_unit : src.units_indexed_) {
                auto& dst_unit = dst.m_renderUnits.emplace_back();

                dst_unit.m_name = src_unit.name_;
                ::convert_material(dst_unit.m_material, src_unit.material_);

                for (const auto index : src_unit.mesh_.indices_){
                    auto& vertex = src_unit.mesh_.vertices_[index];

                    dst_unit.m_mesh.m_vertices.push_back(vertex.pos_.x);
                    dst_unit.m_mesh.m_vertices.push_back(vertex.pos_.y);
                    dst_unit.m_mesh.m_vertices.push_back(vertex.pos_.z);

                    dst_unit.m_mesh.m_texcoords.push_back(vertex.uv_.x);
                    dst_unit.m_mesh.m_texcoords.push_back(vertex.uv_.y);

                    dst_unit.m_mesh.m_normals.push_back(vertex.normal_.x);
                    dst_unit.m_mesh.m_normals.push_back(vertex.normal_.y);
                    dst_unit.m_mesh.m_normals.push_back(vertex.normal_.z);
                }
            }

            for (auto& src_unit : src.units_straight_joint_) {
                auto& dst_unit = dst.m_renderUnits.emplace_back();

                dst_unit.m_name = src_unit.name_;
                ::convert_material(dst_unit.m_material, src_unit.material_);

                dst_unit.m_mesh.m_vertices = src_unit.mesh_.vertices_;
                dst_unit.m_mesh.m_normals = src_unit.mesh_.normals_;
                dst_unit.m_mesh.m_texcoords = src_unit.mesh_.uv_coordinates_;
                dst_unit.m_mesh.m_boneWeights = src_unit.mesh_.joint_weights_;
                dst_unit.m_mesh.m_boneIndex = src_unit.mesh_.joint_indices_;
            }

            for (auto& src_unit : src.units_indexed_joint_) {
                auto& dst_unit = dst.m_renderUnits.emplace_back();

                dst_unit.m_name = src_unit.name_;
                ::convert_material(dst_unit.m_material, src_unit.material_);

                for (const auto index : src_unit.mesh_.indices_){
                    auto& vertex = src_unit.mesh_.vertices_[index];

                    dst_unit.m_mesh.m_vertices.push_back(vertex.pos_.x);
                    dst_unit.m_mesh.m_vertices.push_back(vertex.pos_.y);
                    dst_unit.m_mesh.m_vertices.push_back(vertex.pos_.z);

                    dst_unit.m_mesh.m_texcoords.push_back(vertex.uv_.x);
                    dst_unit.m_mesh.m_texcoords.push_back(1.f - vertex.uv_.y);

                    dst_unit.m_mesh.m_normals.push_back(vertex.normal_.x);
                    dst_unit.m_mesh.m_normals.push_back(vertex.normal_.y);
                    dst_unit.m_mesh.m_normals.push_back(vertex.normal_.z);

                    dst_unit.m_mesh.m_boneIndex.push_back(vertex.joint_indices_.x);
                    dst_unit.m_mesh.m_boneIndex.push_back(vertex.joint_indices_.y);
                    dst_unit.m_mesh.m_boneIndex.push_back(vertex.joint_indices_.z);
                    dst_unit.m_mesh.m_boneIndex.push_back(vertex.joint_indices_.w);

                    dst_unit.m_mesh.m_boneWeights.push_back(vertex.joint_weights_.x);
                    dst_unit.m_mesh.m_boneWeights.push_back(vertex.joint_weights_.y);
                    dst_unit.m_mesh.m_boneWeights.push_back(vertex.joint_weights_.z);
                    dst_unit.m_mesh.m_boneWeights.push_back(vertex.joint_weights_.w);
                }
            }
        }

        // Skeleton
        {
            for (auto& src_joint : src.skeleton_.joints_) {
                const auto jid = dst.m_joints.getOrMakeIndexOf(src_joint.name_);
                auto& dst_joint = dst.m_joints.at(jid);

                dst_joint.setName(src_joint.name_);
                dst_joint.setOffset(src_joint.offset_mat_);
                dst_joint.setParentIndex(src_joint.parent_index_);
                dst_joint.setType(src_joint.joint_type_);
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
            auto& dst_anim = dst.emplace_back(src_anim.name_, src_anim.ticks_per_sec_, src_anim.calc_duration_in_ticks());

            for (size_t i = 0; i < skeleton.getSize(); ++i) {
                auto& joint_info = skeleton.at(i);
                auto src_joint_index = src_anim.find_index_by_name(joint_info.name());

                if (dal::parser::NULL_JID != src_joint_index) {
                    auto& src_joint = src_anim.joints_[src_joint_index];
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
            ::convert_animations(info.m_animations, parsed_model->animations_, info.m_model.m_joints);
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

        return true;
    }

}
