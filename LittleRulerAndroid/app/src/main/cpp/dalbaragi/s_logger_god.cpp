#include "s_logger_god.h"

#include <stdarg.h>

#if defined(_WIN32)
	#include <stdio.h>
	#define printFunc(...) printf(__VA_ARGS__)
#elif defined(__ANDROID__)
	#include <android/log.h>
	#define printFunc(...) __android_log_print(ANDROID_LOG_VERBOSE, "DALBARAGI", __VA_ARGS__)
#else
	#error "Unkown platform"
#endif


namespace dal {

	LoggerGod::LoggerGod(void) {

	}

	void LoggerGod::putFatal(const std::string& text) {
		printFunc("[FATAL] %s\n", text.c_str());
	}

	void LoggerGod::putError(const std::string& text) {
		printFunc("[ERROR] %s\n", text.c_str());
	}

	void LoggerGod::putWarn(const std::string& text) {
		printFunc("[WARN ] %s\n", text.c_str());
	}

	void LoggerGod::putInfo(const std::string& text) {
		printFunc("[INFO ] %s\n", text.c_str());
	}

	void LoggerGod::putDebug(const std::string& text) {
		printFunc("[DEBUG] %s\n", text.c_str());
	}

	void LoggerGod::putTrace(const std::string& text) {
		printFunc("[TRACE] %s\n", text.c_str());
	}

	void LoggerGod::abort(const std::string& text) {
		printFunc("[ABORT] %s\n", text.c_str());
		throw -1;
	}

}