#include "d_camera.h"

#include <glm/gtc/matrix_transform.hpp>


namespace {

    constexpr float clampFPSEulerX(const float x) {
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

    constexpr float clampFPSEulerY(const float y) {
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


// FPSEulerAngles
namespace dal {

    FPSEulerAngles::FPSEulerAngles(void)
        : m_x(0), m_y(0)
    {

    }

    FPSEulerAngles::FPSEulerAngles(const float x, const float y)
        : m_x(x), m_y(y)
    {

    }

    void FPSEulerAngles::setX(const float v) {
        this->m_x = ::clampFPSEulerX(v);
    }

    void FPSEulerAngles::setY(const float v) {
        this->m_y = ::clampFPSEulerY(v);
    }

    void FPSEulerAngles::addX(const float v) {
        this->m_x = ::clampFPSEulerX(this->x() + v);
    }

    void FPSEulerAngles::addY(const float v) {
        this->m_y = ::clampFPSEulerY(this->y() + v);
    }

    void FPSEulerAngles::clampY(const float min, const float max) {
        if ( this->y() > max ) {
            this->m_y = max;
        }
        else if ( this->y() < min ) {
            this->m_y = min;
        }
    }

    glm::mat4 FPSEulerAngles::makeRotateMat(void) const {
        glm::mat4 mat{ 1.0f };
        mat = glm::rotate(mat, this->y(), glm::vec3(-1.0f, 0.0f, 0.0f));
        mat = glm::rotate(mat, this->x(), glm::vec3(0.0f, 1.0f, 0.0f));
        return mat;
    }

    glm::vec3 fpsEuler2vec(const FPSEulerAngles& se) {
        const auto x = se.x();
        const auto y = se.y();

        return glm::vec3{
            sin(x) * cos(y),
            sin(y),
            -cos(x) * cos(y)
        };
    }

    // Header functions

    FPSEulerAngles vec2fpsEuler(glm::vec3 v) {
        const glm::vec3 up{ 0.0f, 1.0f, 0.0f };
        v = glm::normalize(v);

        float y = glm::radians(90.0f) - acos(glm::dot(up, v));

        float x = -atan2(-v.z, v.x) + glm::radians(450.0f);
        x = fmod(x, glm::radians(360.0f));

        return FPSEulerAngles{ x, y };
    }

}


// EulerAnglesFPSCamera
namespace dal {

    void FPSEulerCamera::updateViewMat(void) {
        const auto rot = this->m_fpsEuler.makeRotateMat();
        this->m_viewMat = glm::translate(rot, -this->pos());
    }

    std::pair<glm::vec3, glm::mat4> FPSEulerCamera::makeReflected(const float planeHeight) const {
        dal::FPSEulerCamera newCam = *this;

        auto newPos = newCam.pos();
        newPos.y = 2.0f * planeHeight - newCam.pos().y;
        newCam.setPos(newPos);

        const auto camViewPlane = newCam.getViewPlane();
        newCam.setViewPlane(camViewPlane.x, -camViewPlane.y);

        newCam.updateViewMat();

        return { newCam.pos(), newCam.viewMat() };
    }

    float FPSEulerCamera::calcDirectionXZ(void) const {
        return this->eulerAngles().x();
    }


    glm::vec2 FPSEulerCamera::getViewPlane(void) const {
        return glm::vec2{ this->m_fpsEuler.x(), this->m_fpsEuler.y() };
    }

    void FPSEulerCamera::setViewPlane(const float x, const float y) {
        this->m_fpsEuler.setX(x);
        this->m_fpsEuler.setY(y);
    }


    void FPSEulerCamera::addViewPlane(const float x, const float y) {
        this->m_fpsEuler.addX(x);
        this->m_fpsEuler.addY(y);
    }

}


// FocusCamera
namespace dal {

    void FocusCamera::updateViewMat(void) {
        this->m_viewMat = glm::lookAt(this->pos(), this->m_focusPoint, glm::vec3{ 0, 1, 0 });
    }

    std::pair<glm::vec3, glm::mat4> FocusCamera::makeReflected(const float planeHeight) const {
        dal::FocusCamera newCam = *this;

        auto newPos = newCam.pos();
        newPos.y = 2.0f * planeHeight - newCam.pos().y;
        newCam.setPos(newPos);

        newCam.m_focusPoint.y = 2.0f * planeHeight - newCam.m_focusPoint.y;

        newCam.updateViewMat();

        return { newCam.pos(), newCam.viewMat() };
    }

    float FocusCamera::calcDirectionXZ(void) const {
        const auto direc = this->vecToFocus();
        const float angle = std::atan2(-direc.z, direc.x);
        return glm::radians<float>(90) - angle;
    }

}
