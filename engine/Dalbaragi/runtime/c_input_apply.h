#pragma once

#include <entt/entity/registry.hpp>

#include <d_widget_view.h>

#include "g_charastate.h"


namespace dal {

    class PlayerControlWidget : public dal::Widget2D {

    private:
        class MoveDPad : public dal::Widget2D {
            /*
             * Widget2's width and height always must be same, which means it's always sqaure.
             */

        private:
            static constexpr float CORNER_MARGIN = 40.0f;

        private:
            dal::ColorView m_fixedCenterPoint, m_touchedPoint;
            float m_squareLengthHalf = 10;

            glm::vec2 m_touchedPos{};
            touchID_t m_owning = -1;

        public:
            MoveDPad(dal::Widget2D* const parent, const float winWidth, const float winHeight);

            virtual void render(const float width, const float height, const void* uniloc) override;
            virtual InputDealtFlag onTouch(const dal::TouchEvent& e) override;

            void onParentResize(const float width, const float height);
            void setSquareLength(const float x);

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

    private:
        MoveDPad m_dpad;

        touchID_t m_owningForView;
        glm::vec2 m_lastViewTouchPos{}, m_viewTouchAccum{};

        MoveInputInfo m_keyboardMoveInfo;

    public:
        PlayerControlWidget(const float winWidth, const float winHeight);

        virtual void render(const float width, const float height, const void* uniloc) override;
        virtual InputDealtFlag onTouch(const dal::TouchEvent& e) override;
        virtual InputDealtFlag onKeyInput(const KeyboardEvent& e, const KeyStatesRegistry& keyStates) override;
        virtual void onFocusChange(const bool v) override;

        MoveInputInfo getMoveInfo(const float deltaTime, const float winWidth, const float winHeight);

        void onParentResize(const float width, const float height);
        void setSquareLength(const float x) {
            this->m_dpad.setSquareLength(x);
        }

    private:
        glm::vec2 getMoveVec(void) const;
        glm::vec2 getResetViewAccum(void);

    };

}
