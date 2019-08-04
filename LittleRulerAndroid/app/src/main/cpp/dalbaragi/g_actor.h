#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "p_animation.h"


namespace dal {

    class ModelStatic;
    class ModelAnimated;


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
            AnimationState m_animState;
        };

    }

}


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
        virtual ICharaState* exec(const float deltaTime, const MoveInputInfo& info) = 0;

        const cpnt::Transform& getTransformRef(void) const {
            return this->m_transform;
        }

    };


    class CharaIdleState : public ICharaState {

    public:
        CharaIdleState(cpnt::Transform& transform, cpnt::AnimatedModel& model, dal::StrangeEulerCamera& camera);
        virtual ~CharaIdleState(void) override;

        virtual ICharaState* exec(const float deltaTime, const MoveInputInfo& info) override;

    };


    class CharaWalkState : public ICharaState {

    public:
        CharaWalkState(cpnt::Transform& transform, cpnt::AnimatedModel& model, dal::StrangeEulerCamera& camera);
        virtual ~CharaWalkState(void) override;

        virtual ICharaState* exec(const float deltaTime, const MoveInputInfo& info) override;

    private:
        void applyMove(cpnt::Transform& cpntTrans, cpnt::AnimatedModel& model, const float deltaTime, const MoveInputInfo& info) const;

    };


    namespace cpnt {

        struct CharacterState {
            ICharaState* m_currentState = nullptr ;
        };

    }

}