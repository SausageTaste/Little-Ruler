#pragma once

#include <vector>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "g_actor.h"
#include "p_animation.h"


namespace dal::binfo {

    struct Mesh {
        std::vector<float> m_vertices, m_texcoords, m_normals, m_boneWeights;
        std::vector<int32_t> m_boneIndex;
    };

    struct Material {
        std::string m_diffuseMap, m_roughnessMap, m_metallicMap, m_normalMap;
        glm::vec2 m_texScale{ 1.f, 1.f };
        float m_roughness = 0.3f, m_metallic = 1.f;
    };

    struct RenderUnit {
        std::string m_name;
        Mesh m_mesh;
        Material m_material;
    };

    struct Model {

    public:
        SkeletonInterface m_joints;
        std::vector<RenderUnit> m_renderUnits;
        AABB m_aabb;

    };

}


namespace dal::dlb {

    class IMeshBuilder {

    public:
        virtual ~IMeshBuilder(void) = default;

        virtual const uint8_t* parse(const uint8_t* begin, const uint8_t* const end) = 0;
        virtual void makeVertexArray(std::vector<float>& vertices, std::vector<float>& texcoords, std::vector<float>& normals) const = 0;

    };


    struct RenderUnit {
        std::unique_ptr<IMeshBuilder> m_meshBuilder;
        binfo::Mesh m_mesh;
        binfo::Material m_material;
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
        float m_width, m_height;
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
