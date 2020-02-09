#include "x_mainloop.h"

#include <time.h>

#include <fmt/format.h>

#include <d_logger.h>
#include <d_filesystem.h>

#include "s_configs.h"
#include "o_widget_textbox.h"
#include "g_charastate.h"
#include "p_model.h"
#include "o_widgetcache.h"
#include "u_luascript.h"


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
        : m_resMas(m_task)
        , m_scene(m_resMas, winWidth, winHeight)
        , m_renderMan(m_scene, m_shader, m_resMas, &m_scene.m_playerCam, winWidth, winHeight)
        // Contexts
        , m_contexts(initContexts(winWidth, winHeight, m_shader, m_renderMan, m_scene, m_task))
        , m_currentContext(m_contexts.front().get())
        // Misc
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
        }

        // Misc
        {
            this->m_timer.setCapFPS(0);
            assertUserdataFolder();

            const auto entity = this->m_scene.addObj_static("asset::pbr_ball.dmd");
            auto& transform = this->m_scene.m_entities.get<cpnt::Transform>(entity);
            transform.setPos(0, 3, 0);
        }

        // Test
        {
            test(this->m_timer.getElapsed());
        }
    }

    Mainloop::~Mainloop(void) {

    }

    int Mainloop::update(void) {
        if ( this->m_flagQuit ) {
            return -1;
        }

        const auto deltaTime = m_timer.checkGetElapsed();
        this->m_currentContext = this->m_currentContext->update(deltaTime);

        return 0;
    }

    void Mainloop::onResize(unsigned int width, unsigned int height) {
        GlobalStateGod::getinst().setWinSize(width, height);

        this->m_renderMan.onWinResize(width, height);
        this->m_scene.onResize(width, height);

        for ( auto& cnxt : this->m_contexts ) {
            cnxt->onWinResize(width, height);
        }

        dalVerbose("Resize : {} x {}"_format(width, height));
    }

}
