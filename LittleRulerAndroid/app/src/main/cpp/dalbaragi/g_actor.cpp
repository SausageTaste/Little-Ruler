#include "g_actor.h"

#include "p_resource.h"

#include <glm/gtc/matrix_transform.hpp>


namespace dal {

    void EulerCamera::updateViewMat(void) {
        this->m_viewMat = glm::mat4(1.0f);

        this->m_viewMat = glm::rotate(this->m_viewMat, this->m_viewDirec.y, glm::vec3(-1.0f, 0.0f, 0.0f));
        this->m_viewMat = glm::rotate(this->m_viewMat, this->m_viewDirec.x, glm::vec3(0.0f, 1.0f, 0.0f));
        this->m_viewMat = glm::translate(this->m_viewMat, glm::vec3(-this->m_pos.x, -this->m_pos.y, -this->m_pos.z));
    }


    glm::vec2 EulerCamera::getViewPlane(void) const {
        return this->m_viewDirec;
    }

    void EulerCamera::setViewPlane(const float x, const float y) {
        this->m_viewDirec.x = x;
        this->m_viewDirec.y = y;
    }


    void EulerCamera::addViewPlane(const float x, const float y) {
        this->m_viewDirec.x += x;
        this->m_viewDirec.y += y;

        this->clampViewDir();
    }

    void EulerCamera::clampViewDir(void) {
        constexpr auto plus90Degree = glm::radians(90.0f);
        constexpr auto minus90Degree = glm::radians(-90.0f);

        if ( this->m_viewDirec.y > plus90Degree ) {
            this->m_viewDirec.y = plus90Degree;
        }
        else if ( this->m_viewDirec.y < minus90Degree ) {
            this->m_viewDirec.y = minus90Degree;
        }
    }

}


namespace dal {

    ActorInfo::ActorInfo(const std::string& actorName, const bool flagStatic)
        : m_name(actorName),
        m_static(flagStatic)
    {

    }

    glm::mat4 ActorInfo::getViewMat(void) const {
        //auto scaleMat = glm::scale(glm::mat4{ 1.0f }, { rescale, rescale , rescale });
        auto translateMat = glm::translate(glm::mat4{ 1.0f }, this->m_pos);
        return translateMat * glm::mat4_cast(this->m_quat); // *scaleMat;
    }

    void ActorInfo::rotate(const float v, const glm::vec3& selector) {
        this->m_quat = glm::normalize(glm::angleAxis(v, selector) * this->m_quat);
    }

}


namespace dal {

    Player::Player(EulerCamera* const camera, ActorInfo* actor, Model* model)
        : m_camera(camera),
        m_actor(actor),
        m_model(model)
    {

    }

    EulerCamera* Player::replaceCamera(EulerCamera* const camera) {
        const auto tmp = this->m_camera;
        this->m_camera = camera;
        return tmp;
    }

    ActorInfo* Player::replaceActor(ActorInfo* const actor) {
        const auto tmp = this->m_actor;
        this->m_actor = actor;
        return tmp;
    }

    Model* Player::replaceModel(Model* const model) {
        const auto tmp = this->m_model;
        this->m_model = model;
        return tmp;
    }

}