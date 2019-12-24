#pragma once

#include <list>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "d_daldef.h"
#include "d_opengl_renderunit.h"
#include "d_meshgeo.h"


namespace dal {

    class Transform {

    private:
        struct MatCache {
            bool m_needUpdate = true;
            glm::mat4 m_mat;
        };

    private:
        mutable MatCache m_mat;

        glm::quat m_quat;
        xvec3 m_pos{ 0, 0, 0 };
        float m_scale;
        bool m_isDefault;

    public:
        Transform(void);
        Transform(const xvec3& pos, const glm::quat& quat, const float scale);

        const glm::mat4& transformMat(void) const;
        const xvec3& pos(void) const noexcept {
            return this->m_pos;
        }
        float scale(void) const noexcept {
            return this->m_scale;
        }
        bool isDefault(void) const {
            return this->m_isDefault;
        }

        void setPos(const xvec3& v) noexcept {
            this->m_pos = v;
            this->onValueSet();
        }
        void setPos(const fixed_t x, const fixed_t y, const fixed_t z) noexcept {
            this->m_pos.x = x;
            this->m_pos.y = y;
            this->m_pos.z = z;
            this->onValueSet();
        }
        void addPos(const xvec3& v) {
            this->m_pos += v;
            this->onValueSet();
        }
        void addPos(const fixed_t x, const fixed_t y, const fixed_t z) noexcept {
            this->m_pos.x += x;
            this->m_pos.y += y;
            this->m_pos.z += z;
            this->onValueSet();
        }
        void addPos(const fixed_t v) {
            this->m_pos += v;
            this->onValueSet();
        }

        void setQuat(const glm::quat& q) {
            this->m_quat = q;
            this->onValueSet();
        }
        void setQuat(const float x, const float y, const float z, const float w) {
            this->m_quat.x = x;
            this->m_quat.y = y;
            this->m_quat.z = z;
            this->m_quat.w = w;
            this->onValueSet();
        }
        void rotate(const float v, const glm::vec3& selector);
        void setScale(const float v) {
            this->m_scale = v;
            this->onValueSet();
        }

    private:
        void updateMat(void) const;
        void onValueSet(void) {
            this->m_mat.m_needUpdate = true;
            this->m_isDefault = false;
        }
        bool needUpdate(void) const {
            return this->m_mat.m_needUpdate;
        }

    };

}


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

    private:
        std::list<Camera> m_cameras;
        std::unordered_map<std::string, MeshPack> m_meshes;

        Camera* m_activeCamera = nullptr;

    public:
        Scene(void);

        Camera& activeCam(void) {
            return *this->m_activeCamera;
        }

        MeshPack& addMesh(const std::string& name) {
            auto iter = this->m_meshes.emplace(name, MeshPack{});
            assert(iter.second);
            return iter.first->second;
        }
        void clearMesh(void) {
            this->m_meshes.clear();
        }

        void render(void) const;

    };

}
