#pragma once

#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


namespace dal::v1 {

    // Model

    class AABB {

    public:
        glm::vec3 m_min, m_max;

    };

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

    class PointLight {

    public:
        glm::vec3 m_pos{ 0 }, m_color{ 0 };
        float m_maxDist = 5.f;

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

    };

    class MapChunk {

    public:
        std::vector<RenderUnit> m_renderUnits;
        std::vector<StaticActor> m_staticActors;
        std::vector<WaterPlane> m_waters;

    };

}
