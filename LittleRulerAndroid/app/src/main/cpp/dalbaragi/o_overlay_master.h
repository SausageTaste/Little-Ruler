#pragma once

#include "p_shader_master.h"
#include "o_widgetbase.h"
#include "o_widgetmanager.h"
#include "s_logchannel.h"


namespace dal {

    class OverlayMaster : public iEventHandler {

        //////// Vars ////////

    private:
        const ShaderMaster& m_shaderMas;

        GlobalGameState mGlobalFSM;

        WidgetStorage m_widgets;
        std::vector<Widget2*> m_toDelete;
        WidgetInputDispatcher m_dispatcher;

        float m_winWidth, m_winHeight;

        //////// Funcs ////////

    public:
        OverlayMaster(const OverlayMaster&) = delete;
        OverlayMaster& operator=(const OverlayMaster&) = delete;

    public:
        OverlayMaster(const ShaderMaster& shaderMas, const unsigned int width, const unsigned int height);
        virtual ~OverlayMaster(void) override;

        virtual void onEvent(const EventStatic& e) override;
        void onWinResize(const unsigned int width, const unsigned int height);

        void updateInputs(void);

        void render(void) const;

        void giveWidgetOwnership(Widget2* const w);
        void giveWidgetRef(Widget2* const w);
        bool removeWidgetRef(Widget2* const w);

        void giveBackgroudWidgetRef(Widget2* const w);

    };

}