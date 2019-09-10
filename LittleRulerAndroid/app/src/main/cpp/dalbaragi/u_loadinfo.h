#pragma once

#include <map>
#include <list>
#include <vector>
#include <string>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "g_actor.h"
#include "p_animation.h"


namespace dal::loadedinfo {

    // Primitives

    struct Mesh {
        std::vector<float> m_vertices, m_texcoords, m_normals, m_boneWeights;
        std::vector<int32_t> m_boneIndex;
    };

    struct Material {
        glm::vec3 m_diffuseColor;
        std::string m_diffuseMap, m_specularMap;
        float m_shininess = 32.0f, m_specStrength = 1.0f;
        glm::vec2 m_texSize;
    };

    struct RenderUnit {
        std::string m_name;
        Mesh m_mesh;
        Material m_material;
    };

    // Model infos in maps

    struct IMapItemModel {
        std::string m_modelID;
        std::vector<ActorInfo> m_actors;
    };

    struct ModelDefined : public IMapItemModel {
        RenderUnit m_renderUnit;
        AABB m_boundingBox;
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
        glm::vec3 m_pos, m_depthColor;
        float m_width, m_height, m_shineness, m_specStreng;
        float m_moveSpeed, m_waveStreng, m_darkestDepthPoint, m_reflectivity;
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

    struct Model {

    public:
        SkeletonInterface m_joints;
        std::list<RenderUnit> m_renderUnits;
        AABB m_aabb;
        glm::mat4 m_globalTrans;

    };

}


namespace dal::dlb {

    class IMeshBuilder {

    private:
        template <typename T>
        class Cache {

        private:
            T m_data;
            bool m_ready = false;

        public:
            void set(const T& d) {
                this->m_data = d;
            }
            void set(T&& d) {
                this->m_data = std::move(d);
            }
            T& get(void) {
                dalAssert(this->m_ready);
                return this->m_data;
            }
            const T& get(void) const {
                dalAssert(this->m_ready);
                return this->m_data;
            }
            bool isReady(void) const {
                return this->m_ready;
            }

        };

    public:
        virtual ~IMeshBuilder(void) = default;

        virtual const uint8_t* parse(const uint8_t* begin, const uint8_t* const end) = 0;
        virtual void makeVertexArray(std::vector<float>& vertices, std::vector<float>& texcoords, std::vector<float>& normals) const = 0;

    };


    struct RenderUnit {
        std::unique_ptr<IMeshBuilder> m_meshBuilder;

        struct Mesh {
            std::vector<float> m_vertices, m_texcoords, m_normals;
        } m_mesh;

        struct Material {
            std::string m_diffuseMap, m_specularMap;
            glm::vec3 m_baseColor;
            glm::vec2 m_texScale;
            float m_shininess;
            float m_specStreng;
            bool m_flagAlphaBlend;
        } m_material;
    };


    struct ModelEmbedded {
        std::string m_name;
        std::vector<RenderUnit> m_renderUnits;
        std::vector<ActorInfo> m_staticActors;
        std::unique_ptr<ICollider> m_bounding, m_detailed;
    };


    struct ModelImported {
        std::string m_resourceID;
        std::vector<ActorInfo> m_staticActors;
        bool m_detailedCollider;
    };


    struct WaterPlane {
        glm::vec3 m_centerPos, m_deepColor;
        float m_width, m_height, m_shininess, m_specStreng;
        float m_flowSpeed, m_waveStreng, m_darkestDepth, m_reflectivity;
    };


    struct Plight {
        glm::vec3 m_pos, m_color;
        float m_maxDist;
    };


    struct MapChunkInfo {
        std::vector<ModelEmbedded> m_embeddedModels;
        std::vector<ModelImported> m_importedModels;
        std::vector<WaterPlane> m_waterPlanes;
        std::vector<Plight> m_plights;
    };

}
