#pragma once

#include <string>
#include <memory>
#include <utility>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "m_collider.h"


namespace dal {

    class ModelStatic;
    class ModelAnimated;


    glm::quat rotateQuat(const glm::quat& q, const float radians, const glm::vec3& selector);


    class ICamera {

    public:
        glm::vec3 m_pos;

    protected:
        glm::mat4 m_viewMat;

    public:
        virtual ~ICamera(void) = default;
        virtual void updateViewMat(void) = 0;
        const glm::mat4& getViewMat(void) const {
            return this->m_viewMat;
        }

        virtual std::pair<glm::vec3, glm::mat4> makeReflected(const float planeHeight) const = 0;

    };


    class StrangeEuler {

    private:
        float x, y;

    public:
        StrangeEuler(void);
        StrangeEuler(const float x, const float y);

        float getX(void) const;
        float getY(void) const;
        void setX(const float v);
        void setY(const float v);
        void addX(const float v);
        void addY(const float v);
        void clampY(const float min, const float max);

        glm::mat4 makeRotateMat(void) const;

    };


    glm::vec3 strangeEuler2Vec(const StrangeEuler& se);
    StrangeEuler vec2StrangeEuler(glm::vec3 v);


    class StrangeEulerCamera : public ICamera {

    private:
        StrangeEuler m_stranEuler;

    public:
        virtual void updateViewMat(void) override;
        virtual std::pair<glm::vec3, glm::mat4> makeReflected(const float planeHeight) const override;

        glm::vec2 getViewPlane(void) const;
        StrangeEuler& getStrangeEuler(void);
        const StrangeEuler& getStrangeEuler(void) const;
        void setViewPlane(const float x, const float y);
        void addViewPlane(const float x, const float y);

    };


    class ActorInfo {

    private:
        glm::mat4 m_modelMat;
        std::string m_name;
        glm::quat m_quat;
        glm::vec3 m_pos;
        float m_scale = 1.0f;
        bool m_matNeedUpdate = true;
        bool m_static = true;

    public:
        ActorInfo(void) = default;
        ActorInfo(const std::string& actorName, const bool flagStatic);

        const glm::mat4& getModelMat(void);

        void setQuat(const glm::quat& q);
        void rotate(const float v, const glm::vec3& selector);

        const glm::vec3& getPos(void) const;
        void setPos(const glm::vec3& v);
        void addPos(const glm::vec3& v);

        float getScale(void) const;
        void setScale(const float v);

    };


    constexpr unsigned int MAX_ID_NAME_LEN = 128;


    namespace cpnt {

        struct Transform {
            glm::mat4 m_modelMat;
            glm::quat m_quat;
            glm::vec3 m_pos;
            float m_scale = 1.0f;

            Transform(void);
            void updateMat(void);
        };

        struct Identifier {
            char m_name[MAX_ID_NAME_LEN] = { 0 };
        };

        struct StaticModel {
            ModelStatic* m_model = nullptr;
        };

        struct AnimatedModel {
            ModelAnimated* m_model = nullptr;
        };

    }

}