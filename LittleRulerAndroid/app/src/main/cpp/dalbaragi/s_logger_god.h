#pragma once

#include <mutex>
#include <atomic>
#include <string>
#include <vector>

#include "s_logchannel.h"


#define ENABLE_ASSERT 1


namespace dal {

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


#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define dalVerbose(str) dal::LoggerGod::getinst().putVerbose((str), __LINE__, __func__, __FILE__);
#define dalDebug(str)   dal::LoggerGod::getinst().putDebug((str),   __LINE__, __func__, __FILE__);
#define dalInfo(str)    dal::LoggerGod::getinst().putInfo((str),    __LINE__, __func__, __FILE__);
#define dalWarn(str)    dal::LoggerGod::getinst().putWarn((str),    __LINE__, __func__, __FILE__);
#define dalError(str)   dal::LoggerGod::getinst().putError((str),   __LINE__, __func__, __FILE__);
#define dalFatal(str)   dal::LoggerGod::getinst().putFatal((str),   __LINE__, __func__, __FILE__);
#define dalAbort(str) { dal::LoggerGod::getinst().putFatal((str),   __LINE__, __func__, __FILE__); throw -1; }


#if ENABLE_ASSERT == 1
#define dalAssert(condition) { if (!(condition)) dalAbort("Assertion failed ( " TOSTRING(condition) " ), file " __FILE__ ", line " TOSTRING(__LINE__)); }
#define dalAssertm(condition, message) { if (!(condition)) dalAbort(message); }
#else
#define dalAssert(condition)
#define dalAssertm(condition, message)
#endif