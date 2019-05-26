#pragma once

#include <chrono>


namespace dal {

	float getTime_sec(void);
	void sleepFor(const float v);

	class Timer {

	private:
		uint32_t mDesiredDeltaMicrosec;
		std::chrono::steady_clock::time_point mLastChecked;

	public:
		Timer(void);

		void check(void);
		float check_getElapsed_capFPS(void);
		float getElapsed(void) const;
		bool hasElapsed(float sec) const;

		void setCapFPS(uint32_t v);

	private:
		void waitToCapFPS(void);

	};

}