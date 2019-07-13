#include "x_mainloop.h"

#include <time.h>

#include <fmt/format.h>

#include "s_logger_god.h"
#include "s_configs.h"


using namespace std::string_literals;


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


// Test codes
namespace {

    class RAIITimer {

    private:
        dal::Timer m_timer;

    public:
        ~RAIITimer(void) {
            const auto elapsed = this->m_timer.getElapsed();
            dalVerbose("Test elapsed: "s + std::to_string(elapsed));
        }

    };

    void test(const float deltaTime) {
#ifdef _DEBUG
        constexpr int ITER_COUNT = 1000000;
#else
        constexpr int ITER_COUNT = 100000000;
#endif
    }

}


namespace {

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
        , m_inputApply(m_overlayMas)
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

            auto model = this->m_resMas.orderModelAnimated("asset::model.dae");
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

            LoggerGod::getinst().addChannel(&g_fileLogger);

            script::init(&this->m_renderMan, &this->m_scene);
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

        LoggerGod::getinst().deleteChannel(&g_fileLogger);
    }

    int Mainloop::update(void) {
        if ( m_flagQuit ) return -1;

        const auto deltaTime = m_timer.check_getElapsed();

        if ( this->m_timerForFPSReport.hasElapsed(0.1f) ) {
            this->m_overlayMas.setDisplayedFPS((unsigned int)(1.0f / deltaTime));
            this->m_timerForFPSReport.check();
        }

        this->m_inputApply.apply(deltaTime, this->m_camera, this->m_enttMaster.get<cpnt::Transform>(this->m_player));
        //this->m_scene.applyCollision(*this->m_player.getModel(), *this->m_player.getActor());

        TaskGod::getinst().update();

        auto view = this->m_enttMaster.view<cpnt::AnimatedModel>();
        view.each(
            [](cpnt::AnimatedModel& animatedModel) {
                animatedModel.m_model->updateAnimation0();
            }
        );

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