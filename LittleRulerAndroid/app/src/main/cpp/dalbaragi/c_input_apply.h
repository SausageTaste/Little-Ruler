#pragma once

#include <entt/entity/registry.hpp>

#include "s_event.h"
#include "o_overlay_master.h"
#include "o_widget_texview.h"


namespace dal {

    class InputApplier : public iEventHandler {

    public:
        struct MoveInputInfo {
            glm::vec2 m_view, m_move;
            float m_vertical = 0.0f;

            void clear(void) {
                this->m_view = glm::vec2{ 0.0f };
                this->m_move = glm::vec2{ 0.0f };
                this->m_vertical = 0.0f;
            }
            MoveInputInfo& operator+=(const MoveInputInfo& other) {
                this->m_view += other.m_view;
                this->m_move += other.m_move;
                this->m_vertical += other.m_vertical;

                return *this;
            }
        };

    private:
        class MoveDPad : public dal::Widget2 {
            /*
             * Widget2's width and height always must be same, which means it's always sqaure.
             */

        private:
            dal::ColoredTile m_fixedCenterPoint, m_touchedPoint;

            glm::vec2 m_touchedPos;
            touchID_t m_owning = -1;

        public:
            MoveDPad(dal::Widget2* const parent, const float winWidth, const float winHeight);

            virtual void render(const dal::UnilocOverlay& uniloc, const float width, const float height) override;
            virtual dal::InputCtrlFlag onTouch(const dal::TouchEvent& e) override;
            virtual void onParentResize(const float width, const float height) override;

            glm::vec2 getRel(void) const;
            bool isActive(void) const;

        private:
            void updateTouchedPos(const float x, const float y, glm::vec2& target) const;
            glm::vec2 makeFixedCenterPos(void) const;
            bool isInsideCircle(const glm::vec2& v) const;
            bool isInsideCircle(const float x, const float y) const {
                return this->isInsideCircle(glm::vec2{ x, y });
            }

        };

        class PlayerControlWidget : public dal::Widget2 {

        private:
            MoveDPad m_dpad;

            touchID_t m_owningForView;
            glm::vec2 m_lastViewTouchPos, m_viewTouchAccum;

            MoveInputInfo m_keyboardMoveInfo;

        public:
            PlayerControlWidget(const float winWidth, const float winHeight);

            virtual void render(const dal::UnilocOverlay& uniloc, const float width, const float height) override;
            virtual dal::InputCtrlFlag onTouch(const dal::TouchEvent& e) override;
            virtual InputCtrlFlag onKeyInput(const KeyboardEvent& e, const KeyStatesRegistry& keyStates) override;
            virtual void onParentResize(const float width, const float height) override;

            MoveInputInfo getMoveInfo(const float deltaTime, const float winWidth, const float winHeight);

        private:
            glm::vec2 getMoveVec(void) const;
            glm::vec2 getResetViewAccum(void);

        };

    private:
        GlobalGameState mFSM;
        OverlayMaster& m_overlayMas;
        PlayerControlWidget m_ctrlInputWidget;

    public:
        InputApplier(OverlayMaster& overlayMas, const unsigned int width, const unsigned int height);
        ~InputApplier(void);

        virtual void onEvent(const EventStatic& e) override;

        void apply(const float deltaTime, StrangeEulerCamera& camera, const entt::entity targetEntity, entt::registry& reg);

    };

}