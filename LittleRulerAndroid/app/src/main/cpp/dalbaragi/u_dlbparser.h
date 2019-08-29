#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>

#include <glm/glm.hpp>

#include "m_collider.h"
#include "g_actor.h"


namespace dal {

    namespace dlb {

        struct RenderUnit {
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


        struct MapChunkInfo {
            std::vector<ModelEmbedded> m_embeddedModels;
        };

    }


    std::optional<dlb::MapChunkInfo> parseDLB(const uint8_t* const buf, const size_t bufSize);

}