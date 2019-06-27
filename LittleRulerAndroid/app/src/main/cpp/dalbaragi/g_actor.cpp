#include "g_actor.h"

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>

#include "p_resource.h"
#include "s_logger_god.h"


using namespace fmt::literals;


namespace {

    inline constexpr float clampStrangeEulerX(const float x) {
        constexpr auto plus360Deg = glm::radians(360.0f);

        if ( x >= plus360Deg ) {
            return fmod(x, plus360Deg);
        }
        else if ( x < glm::radians(0.0f) ) {
            return x + glm::trunc(x / plus360Deg) * plus360Deg + plus360Deg;
        }
        else {
            return x;
        }
    }

    inline constexpr float clampStrangeEulerY(const float y) {
        constexpr auto plus90Degree = glm::radians(90.0f);
        constexpr auto minus90Degree = glm::radians(-90.0f);

        if ( y > plus90Degree ) {
            return plus90Degree;
        }
        else if ( y < minus90Degree ) {
            return minus90Degree;
        }
        else {
            return y;
        }
    }

    float getSignedDistance_point2Plane(const glm::vec3& v, const glm::vec4& p) {
        return abs(p.x*v.x + p.y*v.y + p.z*v.z + p.w) * glm::inversesqrt(p.x*p.x + p.y*p.y + p.z+p.z);
    }

}


namespace dal {

    glm::vec3 strangeEuler2Vec(const StrangeEuler& se) {
        const auto x = se.getX();
        const auto y = se.getY();

        return glm::vec3{
            sin(x) * cos(y),
            sin(y),
            -cos(x) * cos(y)
        };
    }

    StrangeEuler vec2StrangeEuler(glm::vec3 v) {
        const glm::vec3 up{ 0.0f, 1.0f, 0.0f };
        v = glm::normalize(v);

        float y = glm::radians(90.0f) - acos(glm::dot(up, v));

        float x = -atan2(-v.z, v.x) + glm::radians(450.0f);
        x = fmod(x, glm::radians(360.0f));

        return StrangeEuler{ x, y };
    }

}


namespace dal {

    StrangeEuler::StrangeEuler(void)
        : x(0.0f), y(0.0f)
    {

    }

    StrangeEuler::StrangeEuler(const float x, const float y)
        : x(x), y(y)
    {

    }

    float StrangeEuler::getX(void) const {
        return this->x;
    }

    float StrangeEuler::getY(void) const {
        return this->y;
    }

    void StrangeEuler::setX(const float v) {
        this->x = clampStrangeEulerX(v);
    }

    void StrangeEuler::setY(const float v) {
        this->y = clampStrangeEulerY(v);
    }

    void StrangeEuler::addX(const float v) {
        this->x = clampStrangeEulerX(this->x + v);
    }

    void StrangeEuler::addY(const float v) {
        this->y = clampStrangeEulerY(this->y + v);
    }

    glm::mat4 StrangeEuler::makeRotateMat(void) const {
        glm::mat4 mat{ 1.0f };
        mat = glm::rotate(mat, this->y, glm::vec3(-1.0f, 0.0f, 0.0f));
        mat = glm::rotate(mat, this->x, glm::vec3(0.0f, 1.0f, 0.0f));
        return mat;
    }

}


namespace dal {

    void StrangeEulerCamera::updateViewMat(void) {
        const auto rot = this->m_stranEuler.makeRotateMat();
        this->m_viewMat = glm::translate(rot, glm::vec3(-this->m_pos.x, -this->m_pos.y, -this->m_pos.z));
    }

    void StrangeEulerCamera::makeReflected(const float planeHeight, glm::vec3& pos, glm::mat4& mat) const {
        dal::StrangeEulerCamera newCam = *this;

        newCam.m_pos.y = 2.0f * planeHeight - newCam.m_pos.y;

        const auto camViewPlane = newCam.getViewPlane();
        newCam.setViewPlane(camViewPlane.x, -camViewPlane.y);

        newCam.updateViewMat();
        
        mat = newCam.getViewMat();
        pos = newCam.m_pos;
    }


    glm::vec2 StrangeEulerCamera::getViewPlane(void) const {
        return glm::vec2{ this->m_stranEuler.getX(), this->m_stranEuler.getY() };
    }

    void StrangeEulerCamera::setViewPlane(const float x, const float y) {
        this->m_stranEuler.setX(x);
        this->m_stranEuler.setY(y);
    }


    void StrangeEulerCamera::addViewPlane(const float x, const float y) {
        this->m_stranEuler.addX(x);
        this->m_stranEuler.addY(y);
    }

}


namespace dal {

    ActorInfo::ActorInfo(const std::string& actorName, const bool flagStatic)
        : m_name(actorName),
        m_static(flagStatic)
    {

    }

    const glm::mat4& ActorInfo::getModelMat(void) {
        if ( this->m_matNeedUpdate ) {
            const auto identity = glm::mat4{ 1.0f };
            const auto scaleMat = glm::scale(identity, glm::vec3{ this->m_scale, this->m_scale , this->m_scale });
            const auto translateMat = glm::translate(identity, this->m_pos);
            this->m_modelMat = translateMat * glm::mat4_cast(this->m_quat) * scaleMat;
            this->m_matNeedUpdate = false;
        }

        return this->m_modelMat;
    }


    void ActorInfo::setQuat(const glm::quat& q) {
        this->m_matNeedUpdate = true;
        this->m_quat = q;
    }

    void ActorInfo::rotate(const float v, const glm::vec3& selector) {
        this->m_matNeedUpdate = true;
        this->m_quat = glm::normalize(glm::angleAxis(v, selector) * this->m_quat);
    }


    const glm::vec3& ActorInfo::getPos(void) const {
        return this->m_pos;
    }

    void ActorInfo::setPos(const glm::vec3& v) {
        this->m_matNeedUpdate = true;
        this->m_pos = v;
    }

    void ActorInfo::addPos(const glm::vec3& v) {
        this->m_matNeedUpdate = true;
        this->m_pos += v;
    }

}


namespace dal {

    Player::Player(StrangeEulerCamera* const camera, ActorInfo* actor, ModelStatic* model)
        : m_camera(camera),
        m_actor(actor),
        m_model(model)
    {

    }

    StrangeEulerCamera* Player::replaceCamera(StrangeEulerCamera* const camera) {
        const auto tmp = this->m_camera;
        this->m_camera = camera;
        return tmp;
    }

    ActorInfo* Player::replaceActor(ActorInfo* const actor) {
        const auto tmp = this->m_actor;
        this->m_actor = actor;
        return tmp;
    }

    ModelStatic* Player::replaceModel(ModelStatic* const model) {
        const auto tmp = this->m_model;
        this->m_model = model;
        return tmp;
    }

}