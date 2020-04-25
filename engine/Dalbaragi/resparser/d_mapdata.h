#pragma once

#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


namespace dal::v1 {

    class AABB {

    public:
        glm::vec3 m_min, m_max;

    };

    // Model

    class Material {

    public:
        std::string m_albedoMap;
        std::string m_roughnessMap;
        std::string m_metallicMap;
        std::string m_normalMap;
        float m_roughness = 0.5f;
        float m_metallic = 1.f;

    };

    class Mesh {

    public:
        std::vector<float> m_vertices, m_uvcoords, m_normals;

    public:
        size_t numVertices(void) const;

    private:
        bool checkSizeValidity(void) const;

    };

    class RenderUnit {

    public:
        Mesh m_mesh;
        Material m_material;
        AABB m_aabb;
        int m_envmapIndex = -1;

    };

    class ModelEmbeded {

    public:
        std::string m_modelID;
        std::vector<RenderUnit> m_renderUnits;

    };

    class ModelImported {

    public:
        std::string m_resourceID;

    };


    // Actor

    namespace cpnt {

        class Transform {

        public:
            glm::vec3 m_pos{ 0 };
            glm::quat m_quat;
            float m_scale = 1.f;

        };

        class Model {

        public:
            std::string m_modelID;

        };

    }

    using component_t = std::variant<
        cpnt::Transform,
        cpnt::Model
    >;

    class StaticActor {

    public:
        std::string m_name;
        int32_t m_modelIndex = -1;
        cpnt::Transform m_trans;

    };

    class DynamicActor {

    public:
        std::string m_name;
        std::vector<component_t> m_components;

    };


    // Misc

    class WaterPlane {

    public:
        glm::vec3 m_centerPos{ 0 };
        glm::vec3 m_deepColor{ 0 };
        float m_width = 1.f, m_height = 1.f;
        float m_flowSpeed;
        float m_waveStreng;
        float m_darkestDepth;
        float m_reflectance;

    };

    struct EnvMap {
        glm::vec3 m_pos;
        std::vector<glm::vec4> m_volume;
    };


    // Lights

    struct ILight {
        std::string m_name;
        glm::vec3 m_color{ 1, 1, 1 };
        float m_intensity = 1000;
        bool m_hasShadow = false;
    };

    struct PointLight : public ILight {
        glm::vec3 m_pos{ 0 };
        float m_maxDist = 5;
        float m_halfIntenseDist = 0;
    };

    struct DirectionalLight : public ILight {
        glm::vec3 m_direction{ 0, -1, 0 };
    };

    struct SpotLight : public ILight {
        glm::vec3 m_pos{ 0 }, m_direction{ 0, -1, 0 };
        float m_maxDist, m_halfIntenseDist;
        float m_spotDegree, m_spotBlend;
    };


    // Map

    class LevelData {

    public:
        struct ChunkData {
            std::string m_name;
            AABB m_aabb;
            glm::vec3 m_offsetPos;
        };

    public:
        std::vector<ChunkData> m_chunks;
        std::vector<DirectionalLight> m_dlights;

    };

    class MapChunk {

    public:
        std::vector<RenderUnit> m_renderUnits;
        std::vector<StaticActor> m_staticActors;
        std::vector<WaterPlane> m_waters;
        std::vector<EnvMap> m_envmaps;

        std::vector<PointLight> m_plights;
        std::vector<SpotLight> m_slights;

    };

}
