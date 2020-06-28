#pragma once

#include "p_animation.h"
#include "m_collider.h"


// Camera, Actor classes
namespace dal {

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

    public:
        Transform m_transform;
        std::string m_name;
        std::vector<std::int32_t> m_envmapIndices;

    public:
        ActorInfo(void) = default;
        ActorInfo(const std::string& actorName);

    };

}


// Components
namespace dal::cpnt {

    constexpr unsigned int MAX_ID_NAME_LEN = 128;


    using Transform = dal::Transform;

    struct Identifier {
        char m_name[MAX_ID_NAME_LEN] = { 0 };
    };

    struct PhysicsObj {
        bool m_touchingGround = false;
    };

}
