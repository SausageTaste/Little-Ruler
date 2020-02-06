#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "d_logchannel.h"


#define ENABLE_ASSERT 1


namespace dal {

    class RefCounter {

          // Possible race condition.
         //  If thread A acquires this while thread B is logging, thread B can still write log.
        //   But I'm gonna leave it as it is because atm only one thread writes log.

    public:
        struct Impl;

    private:
        Impl* pimpl;

    public:
        RefCounter(void);
        RefCounter(const RefCounter&);
        RefCounter& operator=(const RefCounter&);
        RefCounter(RefCounter&& other) noexcept;
        RefCounter& operator=(RefCounter&& other) noexcept;
        ~RefCounter(void);

        size_t getRefCount(void) const;

    };


    class LoggerGod {

    private:
        std::vector<ILoggingChannel*> m_channels;
        std::mutex m_mut;
        RefCounter m_disablingCounter;

    public:
        LoggerGod(const LoggerGod&) = delete;
        LoggerGod(LoggerGod&&) = delete;
        LoggerGod& operator=(const LoggerGod&) = delete;
        LoggerGod& operator=(LoggerGod&&) = delete;

    public:
        LoggerGod(void);
        static LoggerGod& getinst(void) {
            static LoggerGod inst;
            return inst;
        }

        void addChannel(ILoggingChannel* const ch);
        void deleteChannel(ILoggingChannel* const ch);
        RefCounter disable(void);

        void putVerbose(const char* const text, const int line, const char* const func, const char* const file);
        void putDebug(const char* const text, const int line, const char* const func, const char* const file);
        void putInfo(const char* const text, const int line, const char* const func, const char* const file);
        void putWarn(const char* const text, const int line, const char* const func, const char* const file);
        void putError(const char* const text, const int line, const char* const func, const char* const file);
        void putFatal(const char* const text, const int line, const char* const func, const char* const file);

        void putVerbose(const std::string& text, const int line, const char* const func, const char* const file);
        void putDebug(const std::string& text, const int line, const char* const func, const char* const file);
        void putInfo(const std::string& text, const int line, const char* const func, const char* const file);
        void putWarn(const std::string& text, const int line, const char* const func, const char* const file);
        void putError(const std::string& text, const int line, const char* const func, const char* const file);
        void putFatal(const std::string& text, const int line, const char* const func, const char* const file);

    private:
        bool isEnable(void) const;

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