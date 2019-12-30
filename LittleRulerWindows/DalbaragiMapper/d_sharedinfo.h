#pragma once

#include <string>
#include <optional>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "d_daldef.h"


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
        const glm::quat& quat(void) const {
            return this->m_quat;
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

            this->m_quat = glm::normalize(this->m_quat);

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


    class Actor {

    public:
        std::string m_name;
        Transform m_trans;

    };


    class SharedInfo {

    public:
        struct ActiveObject {
            Actor* m_actor = nullptr;
        };

    public:
        ActiveObject m_active;
        bool m_needRedraw = false;

    };

}
