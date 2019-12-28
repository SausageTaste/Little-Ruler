#include "d_sharedinfo.h"

#include <glm/gtc/matrix_transform.hpp>


namespace {

    glm::quat rotateQuat(const glm::quat& q, const float radians, const glm::vec3& selector) {
        return glm::normalize(glm::angleAxis(radians, selector) * q);
    }

}


// Transform
namespace dal {

    Transform::Transform(void)
        : m_scale(1.f)
        , m_isDefault(true)
    {
        this->updateMat();
    }

    Transform::Transform(const xvec3& pos, const glm::quat& quat, const float scale)
        : m_quat(quat)
        , m_pos(pos)
        , m_scale(scale)
        , m_isDefault(false)
    {
        this->updateMat();
    }

    const glm::mat4& Transform::transformMat(void) const {
        if ( this->needUpdate() ) {
            this->updateMat();
        }
        return this->m_mat.m_mat;
    }

    void Transform::rotate(const float v, const glm::vec3& selector) {
        this->m_quat = rotateQuat(this->m_quat, v, selector);
        this->onValueSet();
    }

    // Private

    void Transform::updateMat(void) const {
        const auto identity = glm::mat4{ 1.0f };
        const auto scaleMat = glm::scale(identity, glm::vec3{ this->m_scale, this->m_scale , this->m_scale });
        const auto translateMat = glm::translate(identity, glm::vec3{ this->m_pos });
        this->m_mat.m_mat = translateMat * glm::mat4_cast(this->m_quat) * scaleMat;
        this->m_mat.m_needUpdate = false;
    }

}
