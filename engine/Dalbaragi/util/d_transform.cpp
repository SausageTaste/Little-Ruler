#include "d_transform.h"

#include <glm/gtc/matrix_transform.hpp>

#include "u_math.h"


// Transform
namespace dal {

    Transform::Transform(void)
        : m_scale(1.f)
        , m_isDefault(true)
    {
        this->updateMat();
    }

    Transform::Transform(const glm::vec3& pos, const glm::quat& quat, const float scale)
        : m_quat(quat)
        , m_pos(pos)
        , m_scale(scale)
        , m_isDefault(false)
    {
        this->updateMat();
    }

    const glm::mat4& Transform::getMat(void) const {
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
        const auto translateMat = glm::translate(identity, this->m_pos);
        this->m_mat.m_mat = translateMat * glm::mat4_cast(this->m_quat) * scaleMat;
        this->m_mat.m_needUpdate = false;
    }

}
