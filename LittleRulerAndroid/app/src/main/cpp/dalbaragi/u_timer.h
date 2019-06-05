#pragma once

#include <chrono>
#include <string>


namespace dal {

    float getTime_sec(void);
    void sleepFor(const float v);


    class ScopedTimer {

    private:
        std::chrono::steady_clock::time_point m_constructedTime;
        std::string m_msg;

    public:
        ScopedTimer(const std::string& msg);
        ~ScopedTimer(void);

    };


    class Timer {

    private:
        uint32_t mDesiredDeltaMicrosec;
        std::chrono::steady_clock::time_point mLastChecked;

    public:
        Timer(void);

        void check(void);
        float getElapsed(void) const;
        bool hasElapsed(const float sec) const;

        float check_getElapsed(void);
        float check_getElapsed_capFPS(void);

        void setCapFPS(const uint32_t v);

    private:
        void waitToCapFPS(void);

    };

}