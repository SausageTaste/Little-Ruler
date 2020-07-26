#pragma once

#include <utility>

#include <glm/glm.hpp>


namespace dal {

    class FPSEulerAngles {

    private:
        float m_x, m_y;

    public:
        FPSEulerAngles(void);
        FPSEulerAngles(const float x, const float y);

        float x(void) const {
            return this->m_x;
        }
        float y(void) const {
            return this->m_y;
        }

        void setX(const float v);
        void setY(const float v);
        void addX(const float v);
        void addY(const float v);
        void clampY(const float min, const float max);

        glm::mat4 makeRotateMat(void) const;

    };

    glm::vec3 fpsEuler2vec(const FPSEulerAngles& se);
    FPSEulerAngles vec2fpsEuler(glm::vec3 v);

}


namespace dal {

    class ICamera {

    private:
        glm::vec3 m_pos;

    protected:
        glm::mat4 m_viewMat;

    public:
        virtual ~ICamera(void) = default;

        auto& pos(void) const {
            return this->m_pos;
        }
        const glm::mat4& viewMat(void) const {
            return this->m_viewMat;
        }

        void setPos(const glm::vec3& v) {
            this->m_pos = v;
        }
        void setPos(const float x, const float y, const float z) {
            this->m_pos.x = x;
            this->m_pos.y = y;
            this->m_pos.z = z;
        }
        void addPos(const glm::vec3& v) {
            this->m_pos += v;
        }
        void addPos(const float x, const float y, const float z) {
            this->m_pos.x += x;
            this->m_pos.y += y;
            this->m_pos.z += z;
        }

        virtual void updateViewMat(void) = 0;
        virtual std::pair<glm::vec3, glm::mat4> makeReflected(const float planeHeight) const = 0;

        // 0 if camera is heading to -z direction
        // Value inceases as camera direction rotates cw
        virtual float calcDirectionXZ(void) const = 0;

    };


    class FPSEulerCamera : public ICamera {

    private:
        FPSEulerAngles m_fpsEuler;

    public:
        virtual void updateViewMat(void) override;
        virtual std::pair<glm::vec3, glm::mat4> makeReflected(const float planeHeight) const override;
        virtual float calcDirectionXZ(void) const override;

        glm::vec2 getViewPlane(void) const;
        FPSEulerAngles& eulerAngles(void) {
            return this->m_fpsEuler;
        }
        const FPSEulerAngles& eulerAngles(void) const {
            return this->m_fpsEuler;
        }
        void setViewPlane(const float x, const float y);
        void addViewPlane(const float x, const float y);

    };


    class FocusCamera : public ICamera {

    private:
        glm::vec3 m_focusPoint;

    public:
        virtual void updateViewMat(void) override;
        virtual std::pair<glm::vec3, glm::mat4> makeReflected(const float planeHeight) const override;
        virtual float calcDirectionXZ(void) const override;

        auto& focusPoint(void) {
            return this->m_focusPoint;
        }

        void setFocusPoint(const glm::vec3& v) {
            this->m_focusPoint = v;
        }
        void setFocusPoint(const float x, const float y, const float z) {
            this->m_focusPoint.x = x;
            this->m_focusPoint.y = y;
            this->m_focusPoint.z = z;
        }
        void setFocusPoint(const float v) {
            this->setFocusPoint(v, v, v);
        }

        auto vecToFocus(void) const {
            return this->m_focusPoint - this->pos();
        }

    };

}
