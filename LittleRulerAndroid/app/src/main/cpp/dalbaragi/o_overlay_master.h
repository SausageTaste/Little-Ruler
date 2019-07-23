#pragma once

#include <array>
#include <list>

#include "s_event.h"
#include "p_resource.h"
#include "p_globalfsm.h"
#include "p_shader_master.h"
#include "o_text_cache.h"
#include "o_widget_textbox.h"
#include "o_widget_base.h"
#include "s_logger_god.h"


namespace dal {

    class OverlayMaster : public iEventHandler {

    private:
        class TextStreamChannel : public dal::ILoggingChannel {

        private:
            dal::StringBufferBasic& m_texStream;

        public:
            TextStreamChannel(dal::StringBufferBasic& texStream);

            virtual void verbose(const char* const str, const int line, const char* const func, const char* const file) override;
            virtual void debug(const char* const str, const int line, const char* const func, const char* const file) override;
            virtual void info(const char* const str, const int line, const char* const func, const char* const file) override;
            virtual void warn(const char* const str, const int line, const char* const func, const char* const file) override;
            virtual void error(const char* const str, const int line, const char* const func, const char* const file) override;
            virtual void fatal(const char* const str, const int line, const char* const func, const char* const file) override;

        };

        //////// Vars ////////

    private:
        ResourceMaster& m_resMas;
        const ShaderMaster& m_shaderMas;

        UnicodeCache m_unicodes;

        GlobalGameState mGlobalFSM;

        TextStreamChannel m_texStreamCh;

        // If bool is true, the widget must be deleted in this class.
        std::list<Widget2*> m_widgets2, m_toDelete2;
        Widget2* m_backgroundWidget;
        bool m_backgroundOwned;

        float m_winWidth, m_winHeight;

        //////// Funcs ////////

    public:
        OverlayMaster(const OverlayMaster&) = delete;
        OverlayMaster& operator=(const OverlayMaster&) = delete;

    public:
        OverlayMaster(ResourceMaster& resMas, const ShaderMaster& shaderMas, const unsigned int width, const unsigned int height);
        virtual ~OverlayMaster(void) override;

        virtual void onEvent(const EventStatic& e) override;
        void onWinResize(const unsigned int width, const unsigned int height);

        void updateInputs(void);

        void render(void) const;

        void giveWidgetOwnership(Widget2* const w);
        void giveWidgetRef(Widget2* const w);
        bool removeWidgetRef(Widget2* const w);

        void giveBackgroudWidgetRef(Widget2* const w);
        void giveBackgroudWidgetOwnership(Widget2* const w);

    private:
        void clearBackgroudWidget(void);

    };

}