#include "s_logger_god.h"

#include <algorithm>

#include <fmt/format.h>

#if defined(_WIN32)
#include <iostream>
#elif defined(__ANDROID__)
#include <android/log.h>
#else
#error "Unkown platform"
#endif


using namespace std::string_literals;
using namespace fmt::literals;


namespace {

    const char* const k_packageName = "DALBARAGI";

}


// LogcatChannel
namespace {

    class LogcatChannel : public dal::ILoggingChannel {

    public:
        virtual void verbose(const char* const str, const int line, const char* const func, const char* const file) override {
            const auto text = "[VERBO] "s + str;

#if defined(_WIN32)
            std::cout << text << '\n';
#elif defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_VERBOSE, k_packageName, "%s\n", text.c_str());
#endif
        }

        virtual void debug(const char* const str, const int line, const char* const func, const char* const file) override {
            const auto text = "[DEBUG] "s + str;

#if defined(_WIN32)
            std::cout << text << '\n';
#elif defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_DEBUG, k_packageName, "%s\n", text.c_str());
#endif
        }

        virtual void info(const char* const str, const int line, const char* const func, const char* const file) override {
            auto newText = "[INFO ] "s + str;

#if defined(_WIN32)
            std::cout << newText << '\n';
#elif defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_INFO, k_packageName, "%s\n", newText.c_str());
#endif
        }

        virtual void warn(const char* const str, const int line, const char* const func, const char* const file) override {
            const auto text = "[WARN ] "s + str;

#if defined(_WIN32)
            std::cout << text << '\n';
#elif defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_WARN, k_packageName, "%s\n", text.c_str());
#endif
        }

        virtual void error(const char* const str, const int line, const char* const func, const char* const file) override {
            const auto text = "[ERROR] "s + str;

#if defined(_WIN32)
            std::cout << text << '\n';
#elif defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_ERROR, k_packageName, "%s\n", text.c_str());
#endif
        }

        virtual void fatal(const char* const str, const int line, const char* const func, const char* const file) override {
            const auto text = "[FATAL] "s + str;

#if defined(_WIN32)
            std::cout << text << '\n';
#elif defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_FATAL, k_packageName, "%s\n", text.c_str());
#endif
        }

    } g_logcatCh;

}


// LoggerGod
namespace dal {

    LoggerGod& LoggerGod::getinst(void) {
        static LoggerGod inst;
        return inst;
    }

    LoggerGod::LoggerGod(void)
        : m_enabled(true)
    {
        this->addChannel(&g_logcatCh);
    }

    void LoggerGod::addChannel(ILoggingChannel* const ch) {
        dalAssert(nullptr != ch);

        std::unique_lock<std::mutex> lck{ this->m_mut, std::defer_lock };

        this->m_channels.push_back(ch);
    }

    void LoggerGod::deleteChannel(ILoggingChannel* const ch) {
        dalAssert(nullptr != ch);

        std::unique_lock<std::mutex> lck{ this->m_mut, std::defer_lock };

        const auto found = std::find(this->m_channels.begin(), this->m_channels.end(), ch);
        if ( this->m_channels.end() != found ) {
            this->m_channels.erase(found);
        }
    }

    void LoggerGod::disable(void) {
        this->m_enabled = false;
    }

    void LoggerGod::enable(void) {
        this->m_enabled = true;
    }


    void LoggerGod::putFatal(const std::string& text, const int line, const char* const func, const char* const file) {
        if ( !this->m_enabled ) return;
        std::unique_lock<std::mutex> lck{ this->m_mut, std::defer_lock };

        for ( auto ch : m_channels ) {
            ch->fatal(text.c_str(), line, func, file);
        }
    }

    void LoggerGod::putError(const std::string& text, const int line, const char* const func, const char* const file) {
        if ( !this->m_enabled ) return;
        std::unique_lock<std::mutex> lck{ this->m_mut, std::defer_lock };

        for ( auto ch : m_channels ) {
            ch->error(text.c_str(), line, func, file);
        }
    }

    void LoggerGod::putWarn(const std::string& text, const int line, const char* const func, const char* const file) {
        if ( !this->m_enabled ) return;
        std::unique_lock<std::mutex> lck{ this->m_mut, std::defer_lock };

        for ( auto ch : m_channels ) {
            ch->warn(text.c_str(), line, func, file);
        }
    }

    void LoggerGod::putInfo(const std::string& text, const int line, const char* const func, const char* const file) {
        if ( !this->m_enabled ) return;
        std::unique_lock<std::mutex> lck{ this->m_mut, std::defer_lock };

        for ( auto ch : m_channels ) {
            ch->info(text.c_str(), line, func, file);
        }
    }

    void LoggerGod::putDebug(const std::string& text, const int line, const char* const func, const char* const file) {
        if ( !this->m_enabled ) return;
        std::unique_lock<std::mutex> lck{ this->m_mut, std::defer_lock };

        for ( auto ch : m_channels ) {
            ch->debug(text.c_str(), line, func, file);
        }
    }

    void LoggerGod::putVerbose(const std::string& text, const int line, const char* const func, const char* const file) {
        if ( !this->m_enabled ) return;
        std::unique_lock<std::mutex> lck{ this->m_mut, std::defer_lock };

        for ( auto ch : m_channels ) {
            ch->verbose(text.c_str(), line, func, file);
        }
    }

}