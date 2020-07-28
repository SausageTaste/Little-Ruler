#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


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
        glm::vec3 m_pos;
        float m_scale;
        bool m_isDefault;

    public:
        Transform(void);
        Transform(const glm::vec3& pos, const glm::quat& quat, const float scale);

        const glm::mat4& getMat(void) const;
        const glm::vec3& getPos(void) const noexcept {
            return this->m_pos;
        }
        float getScale(void) const noexcept {
            return this->m_scale;
        }
        bool isDefault(void) const {
            return this->m_isDefault;
        }

        void setPos(const glm::vec3& v) noexcept {
            this->m_pos = v;
            this->onValueSet();
        }
        void setPos(const float x, const float y, const float z) noexcept {
            this->m_pos.x = x;
            this->m_pos.y = y;
            this->m_pos.z = z;
            this->onValueSet();
        }
        void addPos(const glm::vec3& v) {
            this->m_pos += v;
            this->onValueSet();
        }
        void addPos(const float x, const float y, const float z) noexcept {
            this->m_pos.x += x;
            this->m_pos.y += y;
            this->m_pos.z += z;
            this->onValueSet();
        }
        void addPos(const float v) {
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
