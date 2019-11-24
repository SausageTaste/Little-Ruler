#pragma once

#include <chrono>
#include <string>


namespace dal {

    double getTime_sec(void);
    void sleepFor(const double v);


    class Timer {

    private:
        std::chrono::steady_clock::time_point m_lastChecked;

    public:
        Timer(void);

        void check(void);
        double getElapsed(void) const;

        double checkGetElapsed(void);

    protected:
        const std::chrono::steady_clock::time_point& getLastChecked(void) const;

    };


    class TimerThatCaps : public Timer {

    private:
        uint32_t m_desiredDeltaMicrosec;

    public:
        TimerThatCaps(void);

        bool hasElapsed(const double sec) const;

        double check_getElapsed_capFPS(void);

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