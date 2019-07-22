#pragma once

#include <array>

#include <glm/glm.hpp>
#include <entt/entity/registry.hpp>

#include "o_widget_primitive.h"
#include "s_event.h"
#include "p_globalfsm.h"
#include "o_overlay_master.h"
#include "g_actor.h"


namespace dal {

    class InputApplier : public iEventHandler {

    private:
        class MoveDPad : public dal::Widget2 {

        private:
            dal::QuadRenderer m_fixedCenterPoint, m_touchedPoint;

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

        };

        class PlayerControlWidget : public dal::Widget2 {

        private:
            MoveDPad m_dpad;

        public:
            PlayerControlWidget(const float winWidth, const float winHeight);

            virtual void render(const dal::UnilocOverlay& uniloc, const float width, const float height) override;
            virtual dal::InputCtrlFlag onTouch(const dal::TouchEvent& e) override;
            virtual void onParentResize(const float width, const float height) override;

        };

    private:
        GlobalGameState mFSM;
        OverlayMaster& m_overlayMas;
        MoveDPad m_dpadWidget;

    public:
        InputApplier(OverlayMaster& overlayMas, const unsigned int width, const unsigned int height);
        ~InputApplier(void);

        virtual void onEvent(const EventStatic& e) override;

        void apply(const float deltaTime, StrangeEulerCamera& camera, const entt::entity targetEntity, entt::registry& reg);

    };

}