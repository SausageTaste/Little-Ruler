#include "u_timer.h"

#include <string>
#include <thread>

#include <fmt/format.h>

#include "s_logger_god.h"

#define PRINT_OVESRLEEP 0


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
        : mDesiredDeltaMicrosec(0),
        mLastChecked(std::chrono::steady_clock::now())
    {

    }

    void Timer::check(void) {
        this->mLastChecked = std::chrono::steady_clock::now();
    }

    float Timer::check_getElapsed(void) {
        auto now = std::chrono::steady_clock::now();
        auto deltaTime_microsec = (float)std::chrono::duration_cast<std::chrono::microseconds>(now - this->mLastChecked).count();
        this->mLastChecked = now;

        return float(deltaTime_microsec / float(k_microsecBySec));
    }

    float Timer::check_getElapsed_capFPS(void) {
        this->waitToCapFPS();
        return this->check_getElapsed();
    }

    float Timer::getElapsed(void) const {
        auto deltaTime_microsec = (float)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - this->mLastChecked).count();
        return float(deltaTime_microsec / float(k_microsecBySec));
    }

    bool Timer::hasElapsed(const float sec) const {
        return this->getElapsed() >= sec;
    }

    void Timer::setCapFPS(const uint32_t v) {
        if ( 0 == v ) {
            this->mDesiredDeltaMicrosec = 0;
        }
        else {
            mDesiredDeltaMicrosec = k_microsecBySec / v;
        }
    }

    void Timer::waitToCapFPS(void) {
        const auto wakeTime = this->mLastChecked + std::chrono::microseconds{ mDesiredDeltaMicrosec };
#if PRINT_OVESRLEEP == 1
        const auto startSleep = chrono::steady_clock::now();
#endif
        sleepHotUntil(wakeTime);
#if PRINT_OVESRLEEP == 1
        const auto sleepTime = chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - startSleep).count();
        const auto sleepRate = double(sleepTime) / double(mDesiredDeltaMicrosec);
        LoggerGod::getinst().putDebug("Sleep rate: "s + to_string(sleepRate));
#endif
    }

}