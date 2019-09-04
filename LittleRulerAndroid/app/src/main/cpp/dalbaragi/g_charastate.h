#pragma once

#include "g_actor.h"


namespace dal {

    class SceneMaster;


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
        StrangeEulerCamera& m_camera;
        SceneMaster& m_scene;

    public:
        ICharaState(const ICharaState&) = delete;
        ICharaState(ICharaState&&) = delete;
        ICharaState& operator=(const ICharaState&) = delete;
        ICharaState& operator=(ICharaState&&) = delete;

    public:
        ICharaState(cpnt::Transform& transform, cpnt::AnimatedModel& model, StrangeEulerCamera& camera, SceneMaster& scene);
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
            CharacterState(cpnt::Transform& transform, cpnt::AnimatedModel& model, dal::StrangeEulerCamera& camera, SceneMaster& scene);
            void update(const float deltaTime, const MoveInputInfo& info);

        };

    }

}