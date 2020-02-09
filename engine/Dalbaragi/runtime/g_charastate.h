#pragma once

#include "g_actor.h"
#include "p_model.h"


namespace dal {

    class SceneGraph;


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
        StrangeEulerCamera& m_camera;
        SceneGraph& m_scene;

    public:
        ICharaState(const ICharaState&) = delete;
        ICharaState(ICharaState&&) = delete;
        ICharaState& operator=(const ICharaState&) = delete;
        ICharaState& operator=(ICharaState&&) = delete;

    public:
        ICharaState(StrangeEulerCamera& camera, SceneGraph& scene);
        virtual ~ICharaState(void) = default;

        virtual void enter(void) = 0;
        virtual void exit(void) = 0;
        virtual void process(const float deltaTime, const MoveInputInfo& info) = 0;
        virtual ICharaState* exec(const float deltaTime, const MoveInputInfo& info) = 0;

    };


    namespace cpnt {

        class CharacterState {

        private:
            ICharaState* m_currentState;

        public:
            CharacterState(cpnt::Transform& transform, cpnt::AnimatedModel& model, dal::StrangeEulerCamera& camera, SceneGraph& scene);
            void update(const float deltaTime, const MoveInputInfo& info);

        };

    }

}