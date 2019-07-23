#pragma once

#include <chrono>
#include <string>


namespace dal {

    float getTime_sec(void);
    void sleepFor(const float v);


    class Timer {

    private:
        std::chrono::steady_clock::time_point m_lastChecked;

    public:
        Timer(void);

        void check(void);
        float getElapsed(void) const;

        float checkGetElapsed(void);

    protected:
        const std::chrono::steady_clock::time_point& getLastChecked(void) const;

    };


    class TimerThatCaps : public Timer {

    private:
        uint32_t m_desiredDeltaMicrosec;

    public:
        TimerThatCaps(void);

        bool hasElapsed(const float sec) const;

        float check_getElapsed_capFPS(void);

        void setCapFPS(const uint32_t v);

    private:
        void waitToCapFPS(void);

    };


    class ScopedTimer {

    private:
        Timer m_timer;
        std::string m_msg;

    public:
        ScopedTimer(const std::string& msg);
        ~ScopedTimer(void);

    };

}