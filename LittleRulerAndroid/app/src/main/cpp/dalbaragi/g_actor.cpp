#include "g_actor.h"

#include "p_resource.h"

#include <glm/gtc/matrix_transform.hpp>


namespace dal {

    glm::mat4 Camera::makeViewMat(void) const {
        auto viewMat = glm::mat4(1.0f);

        viewMat = glm::rotate(viewMat, this->m_viewDirec.y, glm::vec3(-1.0f, 0.0f, 0.0f));
        viewMat = glm::rotate(viewMat, this->m_viewDirec.x, glm::vec3(0.0f, 1.0f, 0.0f));
        viewMat = glm::translate(viewMat, glm::vec3(-this->m_pos.x, -this->m_pos.y, -this->m_pos.z));

        return viewMat;
    }


    glm::vec3 Camera::getPos(void) const {
        return this->m_pos;
    }

    void Camera::setPos(const float x, const float y, const float z) {
        this->m_pos.x = x;
        this->m_pos.y = y;
        this->m_pos.z = z;
    }

    void Camera::setPos(const glm::vec3& pos) {
        this->m_pos = pos;
    }

    void Camera::addPos(const float x, const float y, const float z) {
        this->m_pos.x += x;
        this->m_pos.y += y;
        this->m_pos.z += z;
    }

    void Camera::addPos(const glm::vec3& pos) {
        this->m_pos += pos;
    }


    glm::vec2 Camera::getViewPlane(void) const {
        return this->m_viewDirec;
    }

    void Camera::setViewPlane(const float x, const float y) {
        this->m_viewDirec.x = x;
        this->m_viewDirec.y = y;
    }


    void Camera::addViewPlane(const float x, const float y) {
        this->m_viewDirec.x += x;
        this->m_viewDirec.y += y;

        this->clampViewDir();
    }

    void Camera::clampViewDir(void) {
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

    Player::Player(Camera* const camera, ActorInfo* actor, Model* model)
        : m_camera(camera),
        m_actor(actor),
        m_model(model)
    {

    }

    Camera* Player::replaceCamera(Camera* const camera) {
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