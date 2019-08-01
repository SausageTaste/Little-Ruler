#include "x_mainloop.h"

#include <time.h>

#include <fmt/format.h>

#include "s_logger_god.h"
#include "s_configs.h"
#include "o_widget_textbox.h"
#include "o_widgetmanager.h"
#include "u_luascript.h"


using namespace std::string_literals;
using namespace fmt::literals;


// Test codes
namespace {

    void test(const float deltaTime) {

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
            dal::LoggerGod::getinst().disable();

            const auto theTime = time(nullptr);
            struct std::tm timeInfo;

#if defined(_WIN32)
            const auto err = localtime_s(&timeInfo, &theTime);
#elif defined(__ANDROID__)
            const auto err = localtime_r(&theTime, &timeInfo);
#endif

            const auto day = timeInfo.tm_mday;
            const auto month = timeInfo.tm_mon + 1; // Month is 0 – 11, add 1 to get a jan-dec 1-12 concept
            const auto year = timeInfo.tm_year + 1900; // Year is # years since 1900
            const auto hour = timeInfo.tm_hour;
            const auto min = timeInfo.tm_min;
            const auto sec = timeInfo.tm_sec;

            std::string buffer{ "Dalbaragi Log\n" };
            buffer += fmt::format("{}-{:0>2}-{:0>2} {:0>2}:{:0>2}:{:0>2}\n\n", year, month, day, hour, min, sec);

            buffer += fmt::format("File : {}\nLine : {}\nFunction : {}\n", file, line, func);
            buffer += "Log level : ";
            buffer += logLevel;
            buffer += "\n\n";

            buffer += "\"\"\"\n";
            buffer += str;
            buffer += "\n\"\"\"";
            buffer += "\n\n##############\n\n";

            const auto fileID = fmt::format("log::log_{}-{:0>2}-{:0>2}_{:0>2}-{:0>2}.txt", year, month, day, hour, min);

            auto logFile = dal::resopen(fileID, dal::FileMode::append);
            if ( nullptr == logFile ) {
                fmt::print("Failed to create log file: {}\n", fileID);
                dal::LoggerGod::getinst().enable();
                return;
            }

            const auto res = logFile->write(buffer.c_str());
            if ( !res ) {
                fmt::print("Failed to write to log file: {}\n", fileID);
                dal::LoggerGod::getinst().enable();
                return;
            }

            dal::LoggerGod::getinst().enable();
        }

    } g_fileLogger;

}


// FPS counter widget
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

        virtual void render(const dal::UnilocOverlay& uniloc, const float width, const float height) {
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
                const auto text = "[VERBO] "s + str + '\n';
                this->m_texStream.append(text.data(), text.size());
            }

            virtual void debug(const char* const str, const int line, const char* const func, const char* const file) override {
                const auto text = "[DEBUG] "s + str + '\n';
                this->m_texStream.append(text.data(), text.size());
            }

            virtual void info(const char* const str, const int line, const char* const func, const char* const file) override {
                const auto text = "[INFO] "s + str + '\n';
                this->m_texStream.append(text.data(), text.size());
            }

            virtual void warn(const char* const str, const int line, const char* const func, const char* const file) override {
                const auto text = "[WARN] "s + str + '\n';
                this->m_texStream.append(text.data(), text.size());
            }

            virtual void error(const char* const str, const int line, const char* const func, const char* const file) override {
                const auto text = "[ERROR] "s + str + '\n';
                this->m_texStream.append(text.data(), text.size());
            }

            virtual void fatal(const char* const str, const int line, const char* const func, const char* const file) override {
                const auto text = "[FATAL] "s + str + '\n';
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

            this->setPos(300.0f, 20.0f);
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

    protected:
        virtual void onScrSpaceBoxUpdate(void) {
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


namespace dal {

    // Static

    void Mainloop::giveWhatFilesystemWants(void* androidAssetManager, const char* const sdcardPath) {
        initFilesystem(androidAssetManager, sdcardPath);
    }

    bool Mainloop::isWhatFilesystemWantsGiven(void) {
        return isFilesystemReady();
    }

    // Public

    Mainloop::Mainloop(const unsigned int winWidth, const unsigned int winHeight)
        : m_flagQuit(false)
        , m_scene(m_resMas, winWidth, winHeight)
        , m_overlayMas(m_resMas, m_shader, winWidth, winHeight)
        , m_renderMan(m_scene, m_shader, m_overlayMas, &m_camera, winWidth, winHeight)
        , m_inputApply(m_overlayMas, winWidth, winHeight)
    {
        // This might be done already by SceneMaster or OverlayMaster but still...
        {
            ConfigsGod::getinst().setWinSize(winWidth, winHeight);
        }

        // Check filesystem init
        {
            if ( !isWhatFilesystemWantsGiven() ) {
                dalAbort("Please call Mainloop::giveWhatFilesystemWants before constructor!");
            }
        }

        // Player
        {
            this->m_player = this->m_enttMaster.create();

            auto& transform = this->m_enttMaster.assign<cpnt::Transform>(this->m_player);
            transform.m_scale = 0.2f;

            auto model = this->m_resMas.orderModelAnimated("asset::Character Running.dae");
            dalAssert(nullptr != model);
            auto& renderable = this->m_enttMaster.assign<cpnt::AnimatedModel>(this->m_player);
            renderable.m_model = model;
        }

        // Camera
        {
            this->m_camera.m_pos = { 0.0f, 3.0f, 3.0f };
        }

        // Regist
        {
            mHandlerName = "dal::Mainloop";
            EventGod::getinst().registerHandler(this, EventType::quit_game);

            this->m_overlayMas.giveWidgetRef(&g_fpsCounter);
            this->m_overlayMas.giveWidgetRef(&g_luaConsole);
        }

        // Misc
        {
            this->m_timer.setCapFPS(0);
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
        if ( m_flagQuit ) return -1;

        const auto deltaTime = m_timer.checkGetElapsed();

        if ( this->m_timerForFPSReport.getElapsed() > 0.1f ) {
            g_fpsCounter.setText(static_cast<unsigned int>(1.0f / deltaTime));
            this->m_timerForFPSReport.check();
        }

        this->m_overlayMas.updateInputs();
        this->m_inputApply.apply(deltaTime, this->m_camera, this->m_player, this->m_enttMaster);
        //this->m_scene.applyCollision(*this->m_player.getModel(), *this->m_player.getActor());

        TaskGod::getinst().update();

        auto view = this->m_enttMaster.view<cpnt::AnimatedModel>();
        for ( const auto entity : view ) {
            auto& cpntModel = view.get(entity);
            auto pModel = cpntModel.m_model;
            updateAnimeState(cpntModel.m_animState, pModel->getAnimations(), pModel->getSkeletonInterf(), pModel->getGlobalInvMat());
        }

        this->m_scene.update(deltaTime);
        this->m_renderMan.update(deltaTime);
        this->m_renderMan.render(this->m_enttMaster);
        this->m_overlayMas.render();

        return 0;
    }

    void Mainloop::onResize(unsigned int width, unsigned int height) {
        ConfigsGod::getinst().setWinSize(width, height);

        this->m_renderMan.onWinResize(width, height);
        this->m_overlayMas.onWinResize(width, height);
        this->m_scene.onResize(width, height);
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