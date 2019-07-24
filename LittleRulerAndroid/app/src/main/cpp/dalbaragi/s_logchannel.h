#pragma once


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

}