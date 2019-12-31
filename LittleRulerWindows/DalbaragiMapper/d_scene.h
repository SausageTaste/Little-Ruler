#pragma once

#include <list>
#include <optional>
#include <unordered_map>

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "d_opengl_renderunit.h"
#include "d_opengl_texture.h"
#include "d_uniloc.h"
#include "d_meshgeo.h"
#include "d_sharedinfo.h"


// Lights
namespace dal {

    class ILight {

    public:
        glm::vec3 m_color{ 0 };
        xvec3 m_pos{ 0 };

    };


    class DirectionalLight : public ILight {

    private:
        glm::vec3 m_direc{ 0, -1, 0 };

    public:
        const glm::vec3 direction(void) const {
            return this->m_direc;
        }
        void setDirection(const float x, const float y, const float z);
        void setDirection(const glm::vec3& direc);

    };

}


// Scene
namespace dal {

    class Scene {

    public:
        class Camera {

        public:
            glm::vec3 m_pos{ 0, 0, 0 };
            glm::vec3 m_viewDir{ 0, 0, -1 };
            glm::vec3 m_upDir{ 0, 1, 0 };

        public:
            glm::mat4 makeViewMat(void) const;

            void rotateVertical(const float angle);
            void rotateHorizontal(const float angle);
            void moveHorizontalAlongView(const glm::vec3& direction);  // (0, 0, -1) is front.

        };

        struct MeshPack {
            gl::Mesh m_glmesh;
            MeshData m_meshdata;
        };

    public:
        entt::registry m_entt;

    private:
        std::list<Camera> m_cameras;

        Camera* m_activeCamera = nullptr;

    public:
        gl::Texture m_albedo, m_roughness, m_metallic;

    public:
        Scene(void);

        Camera& activeCam(void) {
            return *this->m_activeCamera;
        }
        const Camera& activeCam(void) const {
            return *this->m_activeCamera;
        }

        std::optional<entt::entity> castSegment(const Segment& seg);
        void clearGL(void);
        void render(const gl::UniRender_Static& uniloc) const;

    };

}
