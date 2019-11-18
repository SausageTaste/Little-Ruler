﻿#include "x_mainloop.h"

#include <time.h>

#include <fmt/format.h>

#include "s_logger_god.h"
#include "s_configs.h"
#include "o_widget_textbox.h"
#include "o_widgetmanager.h"
#include "u_luascript.h"
#include "g_charastate.h"
#include "p_model.h"
#include "o_widgetcache.h"
#include "u_filesystem.h"


using namespace fmt::literals;


// Utils
namespace {

    struct Datetime {
        int m_year, m_month, m_day, m_hour, m_min, m_sec;
    };

    Datetime getCurrentDatetime(void) {
        Datetime info;

        const auto theTime = time(nullptr);
        struct std::tm timeInfo;

#if defined(_WIN32)
        const auto err = localtime_s(&timeInfo, &theTime);
#elif defined(__ANDROID__)
        const auto err = localtime_r(&theTime, &timeInfo);
#endif
        // TODO : Error handling.

        info.m_day = timeInfo.tm_mday;
        info.m_month = timeInfo.tm_mon + 1;  // Month is 0 – 11, add 1 to get a jan-dec 1-12 concept
        info.m_year = timeInfo.tm_year + 1900;  // Year is # years since 1900
        info.m_hour = timeInfo.tm_hour;
        info.m_min = timeInfo.tm_min;
        info.m_sec = timeInfo.tm_sec;

        return info;
    }

}


// FileLoggingChannel
namespace {

    class FileLoggingChannel : public dal::ILoggingChannel {

    public:
        FileLoggingChannel(void) {
            dal::LoggerGod::getinst().addChannel(this);
        }

        ~FileLoggingChannel(void) {
            dal::LoggerGod::getinst().deleteChannel(this);
        }

        virtual void verbose(const char* const str, const int line, const char* const func, const char* const file) override {

        }

        virtual void debug(const char* const str, const int line, const char* const func, const char* const file) override {

        }

        virtual void info(const char* const str, const int line, const char* const func, const char* const file) override {

        }

        virtual void warn(const char* const str, const int line, const char* const func, const char* const file) override {
            this->saveToFile("warning", str, line, func, file);
        }

        virtual void error(const char* const str, const int line, const char* const func, const char* const file) override {
            this->saveToFile("error", str, line, func, file);
        }

        virtual void fatal(const char* const str, const int line, const char* const func, const char* const file) override {
            this->saveToFile("fatal", str, line, func, file);
        }

    private:
        static void saveToFile(const char* const logLevel, const char* const str, const int line, const char* const func, const char* const file) {
            const auto autoDeleted = dal::LoggerGod::getinst().disable();

            const auto dt = getCurrentDatetime();
            const auto fileID = fmt::format("log::log_{}-{:0>2}-{:0>2}_{:0>2}-{:0>2}.txt", dt.m_year, dt.m_month, dt.m_day, dt.m_hour, dt.m_min);
            const auto fileContents = makeFileContents(dt, logLevel, str, line, func, file);

            dal::assertLogFolder();
            auto logFile = dal::fileopen(fileID.c_str(), dal::FileMode2::append);
            if ( nullptr == logFile ) {
                fmt::print("Failed to create log file: {}\n", fileID);
                return;
            }

            const auto res = logFile->write(fileContents.c_str());
            if ( !res ) {
                fmt::print("Failed to write to log file: {}\n", fileID);
                return;
            }
        }

        static std::string makeFileContents(const Datetime& dt, const char* const logLevel, const char* const str, const int line, const char* const func, const char* const file) {
            static const char* const formatStr =
                "Dalbaragi Log\n"
                "{}-{:0>2}-{:0>2} {:0>2}:{:0>2}:{:0>2}\n"
                "\n"
                "File : {}\n"
                "Line : {}\n"
                "Function : {}\n"
                "Log level : {}\n"
                "\n"
                "========\n"
                "{}\n"
                "========\n"
                "\n"
                "##############\n"
                "\n"
                ;

            return fmt::format(formatStr,
                dt.m_year, dt.m_month, dt.m_day, dt.m_hour, dt.m_min, dt.m_sec, file, line, func, logLevel, str
            );
        }

    } g_fileLogger;

}


// Widgets
namespace {

    class FPSCounter : public dal::Widget2 {

    private:
        dal::Label2 m_label;

    public:
        FPSCounter(void) 
            : dal::Widget2(nullptr)
            , m_label(this)
        {
            this->setPos(10.0f, 10.0f);
            this->setSize(50.0f, 20.0f);
        }

        virtual void render(const dal::UnilocOverlay& uniloc, const float width, const float height) override {
            this->m_label.render(uniloc, width, height);
        }

        void setText(const unsigned int fps) {
            this->m_label.setText(std::to_string(fps));
        }

    protected:
        virtual void onScrSpaceBoxUpdate(void) override {
            this->m_label.setSize(this->getSize());
            this->m_label.setPos(this->getPos());
        };

    } g_fpsCounter;


    class LuaConsole : public dal::Widget2 {

    private:
        class TextStreamChannel : public dal::ILoggingChannel {

        private:
            dal::StringBufferBasic& m_texStream;

        public:
            TextStreamChannel(dal::StringBufferBasic& texStream)
                : m_texStream(texStream)
            {

            }

            virtual void verbose(const char* const str, const int line, const char* const func, const char* const file) override {
                const auto text = "[VERBO] {}\n"_format(str);
                this->m_texStream.append(text.data(), text.size());
            }

            virtual void debug(const char* const str, const int line, const char* const func, const char* const file) override {
                const auto text = "[DEBUG] {}\n"_format(str);
                this->m_texStream.append(text.data(), text.size());
            }

            virtual void info(const char* const str, const int line, const char* const func, const char* const file) override {
                const auto text = "[INFO] {}\n"_format(str);
                this->m_texStream.append(text.data(), text.size());
            }

            virtual void warn(const char* const str, const int line, const char* const func, const char* const file) override {
                const auto text = "[WARN] {}\n"_format(str);
                this->m_texStream.append(text.data(), text.size());
            }

            virtual void error(const char* const str, const int line, const char* const func, const char* const file) override {
                const auto text = "[ERROR] {}\n"_format(str);
                this->m_texStream.append(text.data(), text.size());
            }

            virtual void fatal(const char* const str, const int line, const char* const func, const char* const file) override {
                const auto text = "[FATAL] {}\n"_format(str);
                this->m_texStream.append(text.data(), text.size());
            }

        };

    private:
        dal::WidgetInputDispatcher m_dispatcher;
        dal::LineEdit m_lineEdit;
        dal::TextBox m_textBox;
        glm::vec4 m_bgColor;
        Widget2* m_focused;
        dal::LuaState m_luaState;
        dal::StringBufferBasic m_strbuf;
        TextStreamChannel m_stream;

    public:
        LuaConsole(void)
            : dal::Widget2(nullptr)
            , m_lineEdit(this)
            , m_textBox(this)
            , m_bgColor(0.0f, 0.0f, 0.0f, 1.0f)
            , m_focused(nullptr)
            , m_stream(m_strbuf)
        {
            this->m_lineEdit.setHeight(20.0f);
            this->m_lineEdit.setCallbackOnEnter([this](const char* const text) {
                this->m_luaState.exec(text);
                });

            this->m_luaState.replaceStrbuf(&this->m_strbuf);
            this->m_textBox.replaceBuffer(&this->m_strbuf);
            dal::LoggerGod::getinst().addChannel(&this->m_stream);

            this->setPos(10.0f, 50.0f);
            this->setSize(300.0f, 300.0f);
        }

        ~LuaConsole(void) {
            dal::LoggerGod::getinst().deleteChannel(&this->m_stream);
        }

        virtual void render(const dal::UnilocOverlay& uniloc, const float width, const float height) override {
            const auto [a, b] = this->makeDeviceSpace(width, height);
            dal::renderQuadOverlay(uniloc, a, b, this->m_bgColor);

            this->m_lineEdit.render(uniloc, width, height);
            this->m_textBox.render(uniloc, width, height);
        }

        virtual dal::InputCtrlFlag onTouch(const dal::TouchEvent& e) override {
            dal::Widget2* widgetArr[2] = { &this->m_lineEdit, &this->m_textBox };
            const auto [flag, focused] = this->m_dispatcher.dispatch(widgetArr, widgetArr + 2, e);

            this->m_focused = dal::resolveNewFocus(this->m_focused, focused);

            return flag;
        }

        virtual dal::InputCtrlFlag onKeyInput(const dal::KeyboardEvent& e, const dal::KeyStatesRegistry& keyStates) override {
            if ( &this->m_lineEdit == this->m_focused ) {
                const auto iter = &this->m_focused;
                const auto end = iter + 1;
                return this->m_dispatcher.dispatch(iter, end, e, keyStates);
            }

            return dal::InputCtrlFlag::ignored;
        }

        virtual void onFocusChange(const bool v) override {
            if ( !v ) {
                this->m_lineEdit.onFocusChange(false);
                this->m_textBox.onFocusChange(false);
                this->m_focused = nullptr;
            }
        }

        void exec(const char* const str) {
            this->m_luaState.exec(str);
        }

    protected:
        virtual void onScrSpaceBoxUpdate(void) override {
            constexpr float INNER_MARGIN = 5.0f;

            const auto pp1 = this->getPoint00();
            const auto pp2 = this->getPoint11();

            {
                this->m_lineEdit.setPos(pp1.x + INNER_MARGIN, pp2.y - INNER_MARGIN - this->m_lineEdit.getHeight());
                this->m_lineEdit.setWidth(this->getWidth() - INNER_MARGIN - INNER_MARGIN);
            }

            {
                this->m_textBox.setPos(pp1.x + INNER_MARGIN, pp1.y + INNER_MARGIN);
                this->m_textBox.setSize(
                    this->getWidth() - INNER_MARGIN - INNER_MARGIN,
                    this->getHeight() - this->m_lineEdit.getHeight() - INNER_MARGIN - INNER_MARGIN - INNER_MARGIN
                );
            }
        };

    } g_luaConsole;

}


// Test codes
namespace {

    void test(const float deltaTime) {
      
    }

}


namespace dal {

    // Static

    void Mainloop::giveWhatFilesystemWants(void* androidAssetManager, const char* const sdcardPath) {
        dal::ExternalFuncGod::getinst().giveValue_assetMgr(reinterpret_cast<AAssetManager*>(androidAssetManager));
        dal::ExternalFuncGod::getinst().giveValue_androidStoragePath(sdcardPath);
    }

    bool Mainloop::isWhatFilesystemWantsGiven(void) {
        return true;
    }

    // Public

    Mainloop::Mainloop(const unsigned int winWidth, const unsigned int winHeight)
        // Managers
        : m_scene(m_resMas, winWidth, winHeight)
        , m_overlayMas(m_shader, winWidth, winHeight)
        , m_renderMan(m_scene, m_shader, m_overlayMas, m_resMas, &m_scene.m_playerCam, winWidth, winHeight)
        // Contexts
        , m_currentContext(nullptr)
        , m_cxtIngame(m_renderMan, m_scene, m_overlayMas, winWidth, winHeight)
        // Misc
        , m_frameAccum(0)
        , m_flagQuit(false)
    {
        // This might be done already by SceneGraph or OverlayMaster but still...
        {
            GlobalStateGod::getinst().setWinSize(winWidth, winHeight);
        }

        // Check filesystem init
        {
            if ( !isWhatFilesystemWantsGiven() ) {
                dalAbort("Please call Mainloop::giveWhatFilesystemWants before constructor!");
            }
        }

        // Camera
        {
            this->m_scene.m_playerCam.m_pos = { 0.0f, 3.0f, 3.0f };
        }

        // Widgets
        {
            LuaState::giveDependencies(this, &this->m_renderMan);

            this->m_overlayMas.giveWidgetRef(&g_fpsCounter);
            this->m_overlayMas.giveWidgetRef(&g_luaConsole);
        }

        // Regist
        {
            this->mHandlerName = "dal::Mainloop";
            EventGod::getinst().registerHandler(this, EventType::quit_game);
        }

        // Context
        {
            this->m_currentContext = &this->m_cxtIngame;
        }

        // Misc
        {
            this->m_timer.setCapFPS(0);
            assertUserdataFolder();
        }

        // Test
        {
            test(this->m_timer.getElapsed());
        }
    }

    Mainloop::~Mainloop(void) {
        EventGod::getinst().deregisterHandler(this, EventType::quit_game);

        this->m_overlayMas.removeWidgetRef(&g_fpsCounter);
        this->m_overlayMas.removeWidgetRef(&g_luaConsole);
    }

    int Mainloop::update(void) {
        if ( this->m_flagQuit ) {
            return -1;
        }

        const auto deltaTime = m_timer.checkGetElapsed();

        // FPS counter
        {
            ++this->m_frameAccum;
            const auto elapsedForFPS = this->m_timerForFPSReport.getElapsed();
            if ( elapsedForFPS > 0.1f ) {
                const auto fps = static_cast<unsigned int>(static_cast<float>(this->m_frameAccum) / elapsedForFPS);
                g_fpsCounter.setText(fps);
                this->m_timerForFPSReport.check();
                this->m_frameAccum = 0;
            }
        }

        this->m_currentContext = this->m_currentContext->update(deltaTime);

        return 0;
    }

    void Mainloop::onResize(unsigned int width, unsigned int height) {
        GlobalStateGod::getinst().setWinSize(width, height);

        this->m_renderMan.onWinResize(width, height);
        this->m_overlayMas.onWinResize(width, height);
        this->m_scene.onResize(width, height);

        this->m_cxtIngame.onWinResize(width, height);

        dalVerbose("Resize : {} x {}"_format(width, height));
    }

    void Mainloop::onEvent(const EventStatic& e) {
        switch ( e.type ) {

        case EventType::quit_game:
            this->m_flagQuit = true;
            break;
        default:
            dalWarn("dal::Mainloop can't handle this event:");

        }
    }

}
