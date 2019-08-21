#pragma once


#include "p_animation.h"
#include "m_collider.h"


// Forward declarations
namespace dal {

    class ModelStatic;
    class ModelAnimated;

}


// Header functions
namespace dal {

    glm::quat rotateQuat(const glm::quat& q, const float radians, const glm::vec3& selector);

    /*
    In OpenGL coordinate system, if input is (x, z), rotation follows left hand rule.
    */
    glm::vec2 rotateVec2(const glm::vec2& v, const float radians);

    /*
    Normalize xz components on the plane (0, 1, 0, -v.y).
    So if v.y is longer than length, all 3 components must be resized to make it onto sphere.
    */
    glm::vec3 resizeOnlyXZ(const glm::vec3& v, const float sphereRadius);

    template <unsigned int MAX_LEN>
    glm::vec2 clampVec(glm::vec2 v) {
        constexpr auto maxLen = static_cast<float>(MAX_LEN);
        constexpr auto maxLenSqr = static_cast<float>(MAX_LEN * MAX_LEN);

        const auto lenSqr = glm::dot(v, v);
        if ( lenSqr > maxLenSqr ) {
            v *= glm::inversesqrt(lenSqr) * maxLen;
        }
        return v;
    }
    glm::vec2 clampVec(glm::vec2 v, const float maxLen);

}


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

    private:
        std::string m_name;

    public:
        ActorInfo(void) = default;
        ActorInfo(const std::string& actorName);

    };


    constexpr unsigned int MAX_ID_NAME_LEN = 128;

}


// Components
namespace dal::cpnt {

    using Transform = dal::Transform;

    struct Identifier {
        char m_name[MAX_ID_NAME_LEN] = { 0 };
    };

    struct StaticModel {
        ModelStatic* m_model = nullptr;
    };

    struct AnimatedModel {
        ModelAnimated* m_model = nullptr;
        AnimationState m_animState;
    };

    struct PhysicsObj {
        bool m_touchingGround = false;
    };

}


// Chara states
namespace dal {

    struct MoveInputInfo {

    public:
        glm::vec2 m_view, m_move;
        bool m_jump = false;

    public:
        void merge(const MoveInputInfo& other);
        void clear(void);
        bool hasMovement(void) const;
        bool hasViewMove(void) const;

    };


    class ICharaState {
        // Don't judge me. I love Undertale.

    protected:
        cpnt::Transform& m_transform;
        cpnt::AnimatedModel& m_model;
        dal::StrangeEulerCamera& m_camera;

    public:
        ICharaState(const ICharaState&) = delete;
        ICharaState(ICharaState&&) = delete;
        ICharaState& operator=(const ICharaState&) = delete;
        ICharaState& operator=(ICharaState&&) = delete;

    public:
        ICharaState(cpnt::Transform& transform, cpnt::AnimatedModel& model, dal::StrangeEulerCamera& camera);
        virtual ~ICharaState(void) = default;

        virtual void enter(void) = 0;
        virtual void exit(void) = 0;
        virtual void process(const float deltaTime, const MoveInputInfo& info) = 0;
        virtual ICharaState* exec(const float deltaTime, const MoveInputInfo& info) = 0;

        const cpnt::Transform& getTransformRef(void) const {
            return this->m_transform;
        }

    };


    namespace cpnt {

        class CharacterState {

        private:
            ICharaState* m_currentState;

        public:
            CharacterState(cpnt::Transform& transform, cpnt::AnimatedModel& model, dal::StrangeEulerCamera& camera);
            void update(const float deltaTime, const MoveInputInfo& info);

        };

    }

}