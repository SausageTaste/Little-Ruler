#include "g_actor.h"

#include <memory>

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>

#include "s_logger_god.h"
#include "u_vecutil.h"


using namespace fmt::literals;


// Util functions
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
        constexpr auto plus90Degree = glm::radians(89.9f);
        constexpr auto minus90Degree = glm::radians(-89.9f);

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

}


// Header functions
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


// StrangeEuler
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

    void StrangeEuler::clampY(const float min, const float max) {
        if ( y > max ) {
            this->y = max;
        }
        else if ( y < min ) {
            this->y = min;
        }
    }

    glm::mat4 StrangeEuler::makeRotateMat(void) const {
        glm::mat4 mat{ 1.0f };
        mat = glm::rotate(mat, this->y, glm::vec3(-1.0f, 0.0f, 0.0f));
        mat = glm::rotate(mat, this->x, glm::vec3(0.0f, 1.0f, 0.0f));
        return mat;
    }

}


// StrangeEulerCamera
namespace dal {

    void StrangeEulerCamera::updateViewMat(void) {
        const auto rot = this->m_stranEuler.makeRotateMat();
        this->m_viewMat = glm::translate(rot, glm::vec3(-this->m_pos.x, -this->m_pos.y, -this->m_pos.z));
    }

    std::pair<glm::vec3, glm::mat4> StrangeEulerCamera::makeReflected(const float planeHeight) const {
        dal::StrangeEulerCamera newCam = *this;

        newCam.m_pos.y = 2.0f * planeHeight - newCam.m_pos.y;

        const auto camViewPlane = newCam.getViewPlane();
        newCam.setViewPlane(camViewPlane.x, -camViewPlane.y);

        newCam.updateViewMat();
        
        return std::make_pair(newCam.m_pos, newCam.getViewMat());
    }


    glm::vec2 StrangeEulerCamera::getViewPlane(void) const {
        return glm::vec2{ this->m_stranEuler.getX(), this->m_stranEuler.getY() };
    }

    StrangeEuler& StrangeEulerCamera::getStrangeEuler(void) {
        return this->m_stranEuler;
    }

    const StrangeEuler& StrangeEulerCamera::getStrangeEuler(void) const {
        return this->m_stranEuler;
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


// ActorInfo
namespace dal {

    ActorInfo::ActorInfo(const std::string& actorName)
        : m_name(actorName)
    {

    }

}
