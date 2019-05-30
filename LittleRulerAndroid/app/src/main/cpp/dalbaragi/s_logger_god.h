#pragma once

#include <mutex>
#include <atomic>
#include <string>
#include <vector>


namespace dal {

    class ILoggingChannel {

    public:
        virtual ~ILoggingChannel(void) = default;

        virtual void verbose(const char* const text, const int line, const char* const func, const char* const file) = 0;
        virtual void debug(const char* const text, const int line, const char* const func, const char* const file) = 0;
        virtual void info(const char* const text, const int line, const char* const func, const char* const file) = 0;
        virtual void warn(const char* const text, const int line, const char* const func, const char* const file) = 0;
        virtual void error(const char* const text, const int line, const char* const func, const char* const file) = 0;
        virtual void fatal(const char* const text, const int line, const char* const func, const char* const file) = 0;

    };


    class LoggerGod {

    private:
        std::vector<ILoggingChannel*> m_channels;
        std::mutex m_mut;
        std::atomic_bool m_enabled;

    public:
        LoggerGod(void);
        static LoggerGod& getinst(void);

        void addChannel(ILoggingChannel* const ch);
        void deleteChannel(ILoggingChannel* const ch);
        void disable(void);
        void enable(void);

        void putVerbose(const std::string& text, const int line, const char* const func, const char* const file);
        void putDebug(const std::string& text, const int line, const char* const func, const char* const file);
        void putInfo(const std::string& text, const int line, const char* const func, const char* const file);
        void putWarn(const std::string& text, const int line, const char* const func, const char* const file);
        void putError(const std::string& text, const int line, const char* const func, const char* const file);
        void putFatal(const std::string& text, const int line, const char* const func, const char* const file);

    };

}


#define dalVerbose(str) dal::LoggerGod::getinst().putVerbose((str), __LINE__, __func__, __FILE__);
#define dalDebug(str)   dal::LoggerGod::getinst().putDebug((str),   __LINE__, __func__, __FILE__);
#define dalInfo(str)    dal::LoggerGod::getinst().putInfo((str),    __LINE__, __func__, __FILE__);
#define dalWarn(str)    dal::LoggerGod::getinst().putWarn((str),    __LINE__, __func__, __FILE__);
#define dalError(str)   dal::LoggerGod::getinst().putError((str),   __LINE__, __func__, __FILE__);
#define dalFatal(str)   dal::LoggerGod::getinst().putFatal((str),   __LINE__, __func__, __FILE__);
#define dalAbort(str) { dal::LoggerGod::getinst().putFatal((str),   __LINE__, __func__, __FILE__); throw -1; }