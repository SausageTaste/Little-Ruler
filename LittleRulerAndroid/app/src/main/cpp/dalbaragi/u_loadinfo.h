#pragma once

#include <vector>
#include <string>
#include <list>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "g_actor.h"
#include "m_collider.h"


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

            std::list<loadedinfo::LightDirectional> m_direcLights;
            std::list<loadedinfo::LightPoint> m_pointLights;

            std::list<loadedinfo::WaterPlane> m_waterPlanes;
        };


        // Loaded model infos

        struct ModelStatic {
            std::list<RenderUnit> m_renderUnits;
            AxisAlignedBoundingBox m_aabb;
        };

        struct ModelAnimated {
            std::list<RenderUnitAnimated> m_renderUnits;
            AxisAlignedBoundingBox m_aabb;
        };

    }
}