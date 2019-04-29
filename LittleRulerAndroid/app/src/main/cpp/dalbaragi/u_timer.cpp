#include "u_timer.h"

#include <string>
#include <thread>

#include "s_logger_god.h"

#define PRINT_OVESRLEEP 0


using namespace std;


namespace {

	void sleepHotUntil(const chrono::time_point<chrono::steady_clock>& until) {
		while (chrono::steady_clock::now() < until) {};
	}

	void sleepColdUntil(const chrono::time_point<chrono::steady_clock>& until) {
		this_thread::sleep_until(until);
	}

	void sleepHybridUntil(const chrono::time_point<chrono::steady_clock>& until) {
		const auto delta = chrono::duration_cast<chrono::microseconds>(until - chrono::steady_clock::now()).count() * 1 /4;
		this_thread::sleep_for(chrono::microseconds{ delta });
		while (chrono::steady_clock::now() < until) {};
	}

	constexpr int microsecBySec =    1000000;
	constexpr int nanosecBySec  = 1000000000;

}


namespace dal {

	float getTime_sec(void) {
		return float(chrono::steady_clock::now().time_since_epoch().count()) / float(nanosecBySec);
	}

	void sleepFor(const float v) {
		auto until = chrono::steady_clock::now();
		until += chrono::microseconds{ uint64_t(double(v) * double(microsecBySec)) };
		sleepHybridUntil(until);
	}

}


namespace dal {

	Timer::Timer(void)
		: mDesiredDeltaMicrosec(0),
		mLastChecked(chrono::steady_clock::now())
	{

	}

	Timer::~Timer(void) {

	}

	void Timer::check(void) {
		this->mLastChecked = chrono::steady_clock::now();
	}

	float Timer::check_getElapsed_capFPS(void) {
		this->waitToCapFPS();

		auto now = chrono::steady_clock::now();
		auto deltaTime_microsec = (float)chrono::duration_cast<chrono::microseconds>(now - this->mLastChecked).count();
		this->mLastChecked = now;

		return float(deltaTime_microsec / float(microsecBySec));
	}

	float Timer::getElapsed(void) const {
		auto deltaTime_microsec = (float)chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - this->mLastChecked).count();
		return float(deltaTime_microsec / float(microsecBySec));
	}

	bool Timer::hasElapsed(float sec) const {
		return this->getElapsed() >= sec;
	}

	void Timer::setCapFPS(uint32_t v) {
		mDesiredDeltaMicrosec = microsecBySec / v;
	}

	void Timer::waitToCapFPS(void) {
		const auto wakeTime = this->mLastChecked + chrono::microseconds{ mDesiredDeltaMicrosec };
#if PRINT_OVESRLEEP == 1
		const auto startSleep = chrono::steady_clock::now();
#endif
		sleepHybridUntil(wakeTime);
#if PRINT_OVESRLEEP == 1
		const auto sleepTime = chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - startSleep).count();
		const auto sleepRate = double(sleepTime) / double(mDesiredDeltaMicrosec);
		LoggerGod::getinst().putDebug("Sleep rate: "s + to_string(sleepRate));
#endif
	}

}