#pragma once

#include <map>
#include <list>
#include <vector>
#include <string>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "g_actor.h"
#include "m_collider.h"
#include "p_uniloc.h"


namespace dal {

    constexpr int NUM_BONE_WEIGHT = 3;


    namespace loadedinfo {

        // Primitives

        struct MeshStatic {
            std::vector<float> m_vertices, m_texcoords, m_normals;
        };

        struct MeshAnimated : public MeshStatic {
            std::vector<float> m_boneWeights;  // vec3 each
            std::vector<int32_t> m_boneIndex;  // ivec3 each
        };

        struct Material {
            glm::vec3 m_diffuseColor;
            std::string m_diffuseMap, m_specularMap;
            float m_shininess = 32.0f, m_specStrength = 1.0f;
            glm::vec2 m_texSize;
        };

        struct RenderUnit {
            std::string m_name;
            MeshStatic m_mesh;
            Material m_material;
        };

        struct RenderUnitAnimated {
            std::string m_name;
            MeshAnimated m_mesh;
            Material m_material;
        };

        // Model infos in maps

        struct IMapItemModel {
            std::string m_modelID;
            std::vector<ActorInfo> m_actors;
        };

        struct ModelDefined : public IMapItemModel {
            RenderUnit m_renderUnit;
            AxisAlignedBoundingBox m_boundingBox;
        };

        struct ModelImported : public IMapItemModel {

        };

        struct ModelImportedAnimated : public IMapItemModel {

        };


        // Light infos in maps

        struct ILightItem {
            std::string m_name;
            glm::vec3 m_color;
            bool m_static = true;
        };

        struct LightDirectional : public ILightItem {
            glm::vec3 m_direction;
            float m_halfShadowEdgeSize = 15.0f;
        };

        struct LightPoint : public ILightItem {
            glm::vec3 m_pos;
            float m_maxDist = 0.0f;
        };


        // Misc for maps

        struct WaterPlane {
            glm::vec3 m_pos;
            float width = 0.0f, height = 0.0f;
        };

        struct ImageFileData {
            std::vector<uint8_t> m_buf;
            size_t m_width = 0, m_height = 0, m_pixSize = 0;
        };


        struct LoadedMap {
            std::string m_mapName, m_packageName;

            std::list<loadedinfo::ModelDefined> m_definedModels;
            std::list<loadedinfo::ModelImported> m_importedModels;
            std::list<loadedinfo::ModelImportedAnimated> m_animatedModels;

            std::list<loadedinfo::LightDirectional> m_direcLights;
            std::list<loadedinfo::LightPoint> m_pointLights;

            std::list<loadedinfo::WaterPlane> m_waterPlanes;
        };


        // Loaded model infos

        struct ModelStatic {
            std::list<RenderUnit> m_renderUnits;
            AxisAlignedBoundingBox m_aabb;
        };


        class JointInfoNoParent {

        private:
            std::map<std::string, int32_t> m_map;
            std::vector<glm::mat4> m_boneOffsets;
            std::vector<glm::mat4> m_finalTransform;
            int32_t m_lastMadeIndex = -1;

        public:
            int32_t getIndexOf(const std::string& jointName) const;
            int32_t getOrMakeIndexOf(const std::string& jointName);

            void setOffsetMat(const uint32_t index, const glm::mat4& mat);
            const glm::mat4& getOffsetMat(const uint32_t index) const;

            void setFinalTransform(const uint32_t index, const glm::mat4& mat);
            const glm::mat4& getFinalTransform(const uint32_t index) const;

            bool isEmpty(void) const;

            void sendUniform(const UnilocAnimate& uniloc) const;

        private:
            int32_t upsizeAndGetIndex(void);

        };


        struct ModelAnimated {

        public:
            JointInfoNoParent m_joints;
            std::list<RenderUnitAnimated> m_renderUnits;
            AxisAlignedBoundingBox m_aabb;
            glm::mat4 m_globalTrans;

        };


        struct Animation_old {
            struct JointTransform {
                std::string m_name;
                glm::quat m_quat;
                glm::vec3 m_pos, m_scale;
            };

            struct Keyframe {
                float m_timeStamp = 0.0f;
                std::vector<JointTransform> m_joints;
            };

            std::string m_name;
            std::vector<Keyframe> m_keyframes;
        };


        struct Animation {
            struct JointKeyframes {
                std::map<float, glm::quat> m_rotates;
                std::map<float, glm::vec3> m_poses;
                std::map<float, float> m_scales;
                std::string m_name;
            };

            std::vector<JointKeyframes> m_joints;
            std::string m_name;

            bool hasJoint(const std::string& jointName) const {
                for ( const auto& j : this->m_joints ) {
                    if ( j.m_name == jointName ) {
                        return true;
                    }
                }
                return false;
            }

            const JointKeyframes& getJoint(const std::string& jointName) const {
                for ( const auto& j : this->m_joints ) {
                    if ( j.m_name == jointName ) {
                        return j;
                    }
                }
                throw "Ssibal";
            }
        };

    }  // namespace loadedinfo


    class JointNode {

    private:
        std::string m_name;
        loadedinfo::Animation::JointKeyframes m_keyframe;

        JointNode* m_parent;
        std::vector<JointNode> m_children;
        glm::mat4 m_bindMat, m_invBindMat;

    public:
        JointNode(JointNode* const parent);

        JointNode& newChild(void);
        void setName(const std::string& name);
        void setKeyframe(const loadedinfo::Animation::JointKeyframes& keyframe);
        void setBindMat(const glm::mat4& mat);

        unsigned int getNumNodesRecur(void) const;

        void sendBindPos(const UnilocAnimate& uniloc, const loadedinfo::JointInfoNoParent& jointInterface, glm::mat4 parentMat) const;

    };


    class Animation {

    private:
        std::string m_name;
        JointNode m_rootNode;

    public:
        Animation(const std::string& name, JointNode&& rootNode);

        const std::string& getName(void) const { return this->m_name; }
        unsigned int getNumNodes(void) const;

        void sendBindPos(const UnilocAnimate& uniloc, const loadedinfo::JointInfoNoParent& jointInterface) const;

    };

}