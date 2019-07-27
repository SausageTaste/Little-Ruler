#pragma once

#include "p_resource.h"
#include "p_shader_master.h"
#include "o_widgetbase.h"
#include "o_widgetmanager.h"
#include "u_strbuf.h"
#include "s_logchannel.h"


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

        GlobalGameState mGlobalFSM;

        WidgetStorage m_widgets;
        std::vector<Widget2*> m_toDelete;
        WidgetInputDispatcher m_dispatcher;

        TextStreamChannel m_texStreamCh;

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

    };

}