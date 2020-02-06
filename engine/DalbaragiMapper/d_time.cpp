#include "d_time.h"

#include <thread>
#include <iostream>

#define DAL_PRINT_OVESRLEEP false


namespace {

    void sleepHotUntil(const std::chrono::time_point<std::chrono::steady_clock>& until) {
        while ( std::chrono::steady_clock::now() < until ) {};
    }

    void sleepColdUntil(const std::chrono::time_point<std::chrono::steady_clock>& until) {
        std::this_thread::sleep_until(until);
    }

    void sleepHybridUntil(const std::chrono::time_point<std::chrono::steady_clock>& until) {
        const auto delta = std::chrono::duration_cast<std::chrono::microseconds>(until - std::chrono::steady_clock::now()).count() * 1 /4;
        std::this_thread::sleep_for(std::chrono::microseconds{ delta });
        while ( std::chrono::steady_clock::now() < until ) {};
    }

    constexpr int k_microsecBySec = 1000000;
    constexpr int k_nanosecBySec = 1000000000;
    constexpr double k_nanosecBySec_d = 1000000000.0;

}


// Header functions
namespace dal {

    double getTime_sec(void) {
        return static_cast<double>(std::chrono::steady_clock::now().time_since_epoch().count()) / static_cast<double>(k_nanosecBySec);
    }

    void sleepFor(const double v) {
        auto until = std::chrono::steady_clock::now();
        until += std::chrono::microseconds{ uint64_t(double(v) * double(k_microsecBySec)) };
        sleepHotUntil(until);
    }

}


// class Timer
namespace dal {

    Timer::Timer(void)
        : m_lastChecked(std::chrono::steady_clock::now())
    {

    }

    void Timer::check(void) {
        this->m_lastChecked = std::chrono::steady_clock::now();
    }

    double Timer::getElapsed(void) const {
        const auto deltaTime_microsec = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - this->m_lastChecked).count();
        return static_cast<double>(deltaTime_microsec) / static_cast<double>(k_microsecBySec);
    }

    double Timer::checkGetElapsed(void) {
        const auto now = std::chrono::steady_clock::now();
        const auto deltaTime_microsec = std::chrono::duration_cast<std::chrono::microseconds>(now - this->m_lastChecked).count();
        this->m_lastChecked = now;

        return static_cast<double>(deltaTime_microsec) / static_cast<double>(k_microsecBySec);
    }

    // Protected

    const std::chrono::steady_clock::time_point& Timer::getLastChecked(void) const {
        return this->m_lastChecked;
    }

}


// class TimerThatCaps
namespace dal {

    TimerThatCaps::TimerThatCaps(void)
        : m_desiredDeltaMicrosec(0)
    {

    }

    double TimerThatCaps::check_getElapsed_capFPS(void) {
        this->waitToCapFPS();
        return this->checkGetElapsed();
    }

    bool TimerThatCaps::hasElapsed(const double sec) const {
        return this->getElapsed() >= sec;
    }

    void TimerThatCaps::setCapFPS(const uint32_t v) {
        if ( 0 == v ) {
            this->m_desiredDeltaMicrosec = 0;
        }
        else {
            this->m_desiredDeltaMicrosec = k_microsecBySec / v;
        }
    }

    // Private

    void TimerThatCaps::waitToCapFPS(void) {
        const auto wakeTime = this->getLastChecked() + std::chrono::microseconds{ this->m_desiredDeltaMicrosec };
#if DAL_PRINT_OVESRLEEP
        const auto startSleep = std::chrono::steady_clock::now();
#endif
        sleepHybridUntil(wakeTime);
#if DAL_PRINT_OVESRLEEP
        const auto sleepDurationInMS = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startSleep).count();
        const auto sleepRate = double(sleepDurationInMS) / double(this->m_desiredDeltaMicrosec);
        std::cout << "Actual sleep duration / desired sleep duarion = " << sleepRate << '\n';
#endif
    }

}
