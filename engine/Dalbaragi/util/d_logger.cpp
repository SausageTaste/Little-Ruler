#include "d_logger.h"

#include <atomic>
#include <algorithm>

#include <fmt/format.h>

#if defined(_WIN32)
#include <iostream>
#elif defined(__ANDROID__)
#include <android/log.h>
#else
#error "Unkown platform"
#endif

#include "d_pool.h"


using namespace fmt::literals;


namespace {

    const char* const ANDROID_JAVA_PACKAGE_NAME = "DALBARAGI";

}


// LogcatChannel
namespace {

    class LogcatChannel : public dal::ILoggingChannel {

    public:
        virtual void verbose(const char* const str, const int line, const char* const func, const char* const file) override {
            const auto text = "[VERBO] {}"_format(str);

#if defined(_WIN32)
            std::cout << text << '\n';
#elif defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_VERBOSE, ANDROID_JAVA_PACKAGE_NAME, "%s\n", text.c_str());
#endif
        }

        virtual void debug(const char* const str, const int line, const char* const func, const char* const file) override {
            const auto text = "[DEBUG] {}"_format(str);

#if defined(_WIN32)
            std::cout << text << '\n';
#elif defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_DEBUG, ANDROID_JAVA_PACKAGE_NAME, "%s\n", text.c_str());
#endif
        }

        virtual void info(const char* const str, const int line, const char* const func, const char* const file) override {
            auto newText = "[INFO ] {}"_format(str);

#if defined(_WIN32)
            std::cout << newText << '\n';
#elif defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_INFO, ANDROID_JAVA_PACKAGE_NAME, "%s\n", newText.c_str());
#endif
        }

        virtual void warn(const char* const str, const int line, const char* const func, const char* const file) override {
            const auto text = "[WARN ] {}"_format(str);

#if defined(_WIN32)
            std::cout << text << '\n';
#elif defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_WARN, ANDROID_JAVA_PACKAGE_NAME, "%s\n", text.c_str());
#endif
        }

        virtual void error(const char* const str, const int line, const char* const func, const char* const file) override {
            const auto text = "[ERROR] {}"_format(str);

#if defined(_WIN32)
            std::cout << text << '\n';
#elif defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_ERROR, ANDROID_JAVA_PACKAGE_NAME, "%s\n", text.c_str());
#endif
        }

        virtual void fatal(const char* const str, const int line, const char* const func, const char* const file) override {
            const auto text = "[FATAL] {}"_format(str);

#if defined(_WIN32)
            std::cout << text << '\n';
#elif defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_FATAL, ANDROID_JAVA_PACKAGE_NAME, "%s\n", text.c_str());
#endif
        }

    } g_logcatCh;

}


// RefCounter
namespace dal {

    struct RefCounter::Impl {
        std::atomic_size_t m_refCount = 0;
    };

    StaticPool<RefCounter::Impl, 10> g_refCounterPool;


    RefCounter::RefCounter(void)
        : pimpl(g_refCounterPool.alloc())
    {
        this->pimpl->m_refCount++;
    }

    RefCounter::RefCounter(const RefCounter& other)
        : pimpl(other.pimpl)
    {
        this->pimpl->m_refCount++;
    }


    RefCounter::RefCounter(RefCounter&& other) noexcept
        : pimpl(other.pimpl)
    {
        other.pimpl = nullptr;
    }

    RefCounter& RefCounter::operator=(const RefCounter& other) {
        this->pimpl = other.pimpl;

        this->pimpl->m_refCount++;

        return *this;
    }

    RefCounter& RefCounter::operator=(RefCounter&& other) noexcept {
        this->pimpl = other.pimpl;
        other.pimpl = nullptr;

        return *this;
    }

    RefCounter::~RefCounter(void) {
        if ( nullptr != this->pimpl ) {
            this->pimpl->m_refCount--;

            if ( 0 == this->pimpl->m_refCount ) {
                g_refCounterPool.free(this->pimpl);
            }

            this->pimpl = nullptr;
        }
    }

    size_t RefCounter::getRefCount(void) const {
        return this->pimpl->m_refCount;
    }

}


// LoggerGod
namespace dal {

    LoggerGod::LoggerGod(void) {
        this->addChannel(&g_logcatCh);
    }

    void LoggerGod::addChannel(ILoggingChannel* const ch) {
        dalAssert(nullptr != ch);

        std::unique_lock<std::mutex> lck{ this->m_mut, std::defer_lock };

        if ( std::find(this->m_channels.begin(), this->m_channels.end(), ch) != this->m_channels.end() ) {
            dalWarn("Tried to add a logger channel that has been already added.");
            return;
        }

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

    RefCounter LoggerGod::disable(void) {
        return this->m_disablingCounter;
    }


    void LoggerGod::putVerbose(const char* const text, const int line, const char* const func, const char* const file) {
        if ( !this->isEnable() ) {
            return;
        }

        std::unique_lock<std::mutex> lck{ this->m_mut, std::defer_lock };

        for ( auto ch : this->m_channels ) {
            ch->verbose(text, line, func, file);
        }
    }

    void LoggerGod::putDebug(const char* const text, const int line, const char* const func, const char* const file) {
        if ( !this->isEnable() ) {
            return;
        }

        std::unique_lock<std::mutex> lck{ this->m_mut, std::defer_lock };

        for ( auto ch : this->m_channels ) {
            ch->debug(text, line, func, file);
        }
    }

    void LoggerGod::putInfo(const char* const text, const int line, const char* const func, const char* const file) {
        if ( !this->isEnable() ) {
            return;
        }

        std::unique_lock<std::mutex> lck{ this->m_mut, std::defer_lock };

        for ( auto ch : this->m_channels ) {
            ch->info(text, line, func, file);
        }
    }

    void LoggerGod::putWarn(const char* const text, const int line, const char* const func, const char* const file) {
        if ( !this->isEnable() ) {
            return;
        }

        std::unique_lock<std::mutex> lck{ this->m_mut, std::defer_lock };

        for ( auto ch : this->m_channels ) {
            ch->warn(text, line, func, file);
        }
    }

    void LoggerGod::putError(const char* const text, const int line, const char* const func, const char* const file) {
        if ( !this->isEnable() ) {
            return;
        }

        std::unique_lock<std::mutex> lck{ this->m_mut, std::defer_lock };

        for ( auto ch : this->m_channels ) {
            ch->error(text, line, func, file);
        }
    }

    void LoggerGod::putFatal(const char* const text, const int line, const char* const func, const char* const file) {
        if ( !this->isEnable() ) {
            return;
        }

        std::unique_lock<std::mutex> lck{ this->m_mut, std::defer_lock };

        for ( auto ch : this->m_channels ) {
            ch->fatal(text, line, func, file);
        }
    }


    void LoggerGod::putFatal(const std::string& text, const int line, const char* const func, const char* const file) {
        this->putFatal(text.c_str(), line, func, file);
    }

    void LoggerGod::putError(const std::string& text, const int line, const char* const func, const char* const file) {
        this->putError(text.c_str(), line, func, file);
    }

    void LoggerGod::putWarn(const std::string& text, const int line, const char* const func, const char* const file) {
        this->putWarn(text.c_str(), line, func, file);
    }

    void LoggerGod::putInfo(const std::string& text, const int line, const char* const func, const char* const file) {
        this->putInfo(text.c_str(), line, func, file);
    }

    void LoggerGod::putDebug(const std::string& text, const int line, const char* const func, const char* const file) {
        this->putDebug(text.c_str(), line, func, file);
    }

    void LoggerGod::putVerbose(const std::string& text, const int line, const char* const func, const char* const file) {
        this->putVerbose(text.c_str(), line, func, file);
    }

    // Private

    bool LoggerGod::isEnable(void) const {
        return this->m_disablingCounter.getRefCount() <= 1;
    }

}