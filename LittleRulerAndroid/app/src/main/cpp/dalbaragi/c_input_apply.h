#pragma once

#include <array>

#include <glm/glm.hpp>

#include "o_widget_primitive.h"
#include "s_event.h"
#include "p_globalfsm.h"
#include "o_overlay_master.h"
#include "g_actor.h"


namespace dal {

    class InputApplier : public iEventHandler {

    private:
        GlobalGameState mFSM;
        OverlayMaster& m_overlayMas;

    public:
        InputApplier(OverlayMaster& overlayMas);
        ~InputApplier(void);

        virtual void onEvent(const EventStatic& e) override;

        void apply(const float deltaTime, Camera& camera);

    };

}