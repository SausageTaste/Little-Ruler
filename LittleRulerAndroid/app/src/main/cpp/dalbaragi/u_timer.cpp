#include "u_timer.h"

#include <string>
#include <thread>

#include <fmt/format.h>

#include "s_logger_god.h"

#define PRINT_OVESRLEEP 1


using namespace std::string_literals;
using namespace fmt::literals;


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


namespace dal {

    float getTime_sec(void) {
        return float(std::chrono::steady_clock::now().time_since_epoch().count()) / float(k_nanosecBySec);
    }

    void sleepFor(const float v) {
        auto until = std::chrono::steady_clock::now();
        until += std::chrono::microseconds{ uint64_t(double(v) * double(k_microsecBySec)) };
        sleepHotUntil(until);
    }

}


namespace dal {

    ScopedTimer::ScopedTimer(const std::string& msg)
        : m_constructedTime(std::chrono::steady_clock::now()),
        m_msg(msg)
    {

    }

    ScopedTimer::~ScopedTimer(void) {
        const auto uptime = std::chrono::steady_clock::now() - this->m_constructedTime;
        const auto what = static_cast<double>(uptime.count()) / k_nanosecBySec_d;
        dalDebug("ScopedTimer{{ {} }} ({}): "_format(this->m_msg, what));
    }

}


namespace dal {

    Timer::Timer(void)
        : m_lastChecked(std::chrono::steady_clock::now())
    {

    }

    void Timer::check(void) {
        this->m_lastChecked = std::chrono::steady_clock::now();
    }

    float Timer::getElapsed(void) const {
        const auto deltaTime_microsec = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - this->m_lastChecked).count();
        return static_cast<float>(deltaTime_microsec) / static_cast<float>(k_microsecBySec);
    }

    float Timer::checkGetElapsed(void) {
        const auto now = std::chrono::steady_clock::now();
        const auto deltaTime_microsec = std::chrono::duration_cast<std::chrono::microseconds>(now - this->m_lastChecked).count();
        this->m_lastChecked = now;

        return static_cast<float>(deltaTime_microsec) / static_cast<float>(k_microsecBySec);
    }

    const std::chrono::steady_clock::time_point& Timer::getLastChecked(void) const {
        return this->m_lastChecked;
    }

}


namespace dal {

    TimerThatCaps::TimerThatCaps(void)
        : m_desiredDeltaMicrosec(0)
    {

    }

    float TimerThatCaps::check_getElapsed_capFPS(void) {
        this->waitToCapFPS();
        return this->checkGetElapsed();
    }

    bool TimerThatCaps::hasElapsed(const float sec) const {
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

    void TimerThatCaps::waitToCapFPS(void) {
        const auto wakeTime = this->getLastChecked() + std::chrono::microseconds{ this->m_desiredDeltaMicrosec };
#if PRINT_OVESRLEEP == 1
        const auto startSleep = std::chrono::steady_clock::now();
#endif
        sleepHotUntil(wakeTime);
#if PRINT_OVESRLEEP == 1
        const auto sleepTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startSleep).count();
        const auto sleepRate = double(sleepTime) / double(this->m_desiredDeltaMicrosec);
        dalDebug("Sleep rate: "s + std::to_string(sleepRate));
#endif
    }

}