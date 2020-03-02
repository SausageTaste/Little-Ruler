#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


namespace dal::v1 {

    struct Mesh {
        std::vector<float> m_vertices, m_uvCoords, m_normals;
    };

    struct Material {
        std::string m_diffuseMap, m_roughnessMap, m_metallicMap;
        glm::vec2 m_texScale;
        float m_roughness, m_metallic;
    };

    struct RenderUnit {
        Material m_material;
        Mesh m_mesh;
    };

    struct Transform {
        glm::vec3 m_pos;
        glm::quat m_rot;
        float m_scale;
    };

    struct StaticActor {
        std::string m_name;
        Transform m_trans;
    };

    struct ModelEmbedded {
        std::string m_name;
        std::vector<RenderUnit> m_renderUnit;
        std::vector<StaticActor> m_staticActors;
        bool m_detailedCollider, m_hasRotatingActor;
    };

    struct ModelImported {
        std::string m_resID;
        std::vector<StaticActor> m_staticActors;
        bool m_detailedCollider;
    };

    struct WaterPlane {
        glm::vec3 m_pos, m_color;
        float m_width, m_height, m_flowSpeed, m_waveStrength;
        float m_darkestDepth, m_reflectance;
    };

    struct PointLight {
        glm::vec3 m_color, m_pos;
        float m_maxDistance;
    };

    struct MapChunkInfo {
        std::vector<ModelEmbedded> m_embeddedModels;
        std::vector<ModelImported> m_importedModels;
        std::vector<WaterPlane> m_waterPlanes;
        std::vector<PointLight> m_plights;
    };

}
