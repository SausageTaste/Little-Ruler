#include "s_logger_god.h"

#include <stdarg.h>

#if defined(_WIN32)
	#include <stdio.h>

	#define printVerbose(...) printf(__VA_ARGS__)
	#define printDebug(...)   printf(__VA_ARGS__)
	#define printInfo(...)    printf(__VA_ARGS__)
	#define printWarn(...)    printf(__VA_ARGS__)
	#define printError(...)   printf(__VA_ARGS__)
	#define printAssert(...)  printf(__VA_ARGS__)
#elif defined(__ANDROID__)
	#include <android/log.h>

	#define printVerbose(...) __android_log_print(ANDROID_LOG_VERBOSE, "DALBARAGI", __VA_ARGS__)
	#define printDebug(...)   __android_log_print(ANDROID_LOG_DEBUG, "DALBARAGI", __VA_ARGS__)
	#define printInfo(...)    __android_log_print(ANDROID_LOG_INFO, "DALBARAGI", __VA_ARGS__)
	#define printWarn(...)    __android_log_print(ANDROID_LOG_WARN, "DALBARAGI", __VA_ARGS__)
	#define printError(...)   __android_log_print(ANDROID_LOG_ERROR, "DALBARAGI", __VA_ARGS__)
	#define printAssert(...)  __android_log_print(ANDROID_LOG_ASSERT, "DALBARAGI", __VA_ARGS__)
#else
	#error "Unkown platform"
#endif


namespace dal {

	LoggerGod::LoggerGod(void) {

	}

	void LoggerGod::putFatal(const std::string& text) {
		printError("[FATAL] %s\n", text.c_str());
	}

	void LoggerGod::putError(const std::string& text) {
		printError("[ERROR] %s\n", text.c_str());
	}

	void LoggerGod::putWarn(const std::string& text) {
		printWarn("[WARN ] %s\n", text.c_str());
	}

	void LoggerGod::putInfo(const std::string& text) {
		printInfo("[INFO ] %s\n", text.c_str());
	}

	void LoggerGod::putDebug(const std::string& text) {
		printDebug("[DEBUG] %s\n", text.c_str());
	}

	void LoggerGod::putTrace(const std::string& text) {
		printVerbose("[TRACE] %s\n", text.c_str());
	}

	void LoggerGod::abort(const std::string& text) {
		printError("[ABORT] %s\n", text.c_str());
		throw -1;
	}

}