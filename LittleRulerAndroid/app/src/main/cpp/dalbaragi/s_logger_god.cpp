#include "s_logger_god.h"

#if defined(_WIN32)
	#include <iostream>
#elif defined(__ANDROID__)
	#include <android/log.h>
#else
	#error "Unkown platform"
#endif


using namespace std::string_literals;


namespace {

	const char* const k_packageName = "DALBARAGI";

}


// LogcatChannel
namespace {

	class LogcatChannel : public dal::ILoggingChannel {

	public:
		virtual void verbose(const char* const) override;
		virtual void debug(const char* const) override;
		virtual void info(const char* const) override;
		virtual void warn(const char* const) override;
		virtual void error(const char* const) override;
		virtual void fatal(const char* const) override;

	};


	void LogcatChannel::verbose(const char* const str) {
		const auto text = "[VERBO]"s + str;

#if defined(_WIN32)
		std::cout << text << '\n';
#elif defined(__ANDROID__)
		__android_log_print(ANDROID_LOG_VERBOSE, k_packageName, "%s\n", text.c_str());
#endif

	}

	void LogcatChannel::debug(const char* const str) {
		const auto text = "[DEBUG]"s + str;

#if defined(_WIN32)
		std::cout << text << '\n';
#elif defined(__ANDROID__)
		__android_log_print(ANDROID_LOG_DEBUG, k_packageName, "%s\n", text.c_str());
#endif

	}

	void LogcatChannel::info(const char* const str) {
		const auto text = "[INFO ]"s + str;

#if defined(_WIN32)
		std::cout << text << '\n';
#elif defined(__ANDROID__)
		__android_log_print(ANDROID_LOG_INFO, k_packageName, "%s\n", text.c_str());
#endif

	}

	void LogcatChannel::warn(const char* const str) {
		const auto text = "[WARN ]"s + str;

#if defined(_WIN32)
		std::cout << text << '\n';
#elif defined(__ANDROID__)
		__android_log_print(ANDROID_LOG_WARN, k_packageName, "%s\n", text.c_str());
#endif

	}

	void LogcatChannel::error(const char* const str) {
		const auto text = "[ERROR]"s + str;

#if defined(_WIN32)
		std::cout << text << '\n';
#elif defined(__ANDROID__)
		__android_log_print(ANDROID_LOG_ERROR, k_packageName, "%s\n", text.c_str());
#endif

	}

	void LogcatChannel::fatal(const char* const str) {
		const auto text = "[FATAL]"s + str;

#if defined(_WIN32)
		std::cout << text << '\n';
#elif defined(__ANDROID__)
		__android_log_print(ANDROID_LOG_FATAL, k_packageName, "%s\n", text.c_str());
#endif

	}

}


// LoggerGod
namespace dal {

	LoggerGod& LoggerGod::getinst(void) {
		static LoggerGod inst;
		return inst;
	}

	LoggerGod::LoggerGod(void) {
		this->giveChannel(new LogcatChannel);
	}

	LoggerGod::~LoggerGod(void) {
		for (auto ch : this->m_privateChannels) {
			delete ch;
		}
	}

	void LoggerGod::addChannel(ILoggingChannel* const ch) {
		this->m_channels.push_back(ch);
	}

	void LoggerGod::giveChannel(ILoggingChannel* const ch) {
		this->m_privateChannels.emplace_back(ch);
	}


	void LoggerGod::putFatal(const std::string& text) {
		for (auto ch : m_channels) {
			ch->fatal(text.c_str());
		}

		for (auto ch : this->m_privateChannels) {
			ch->fatal(text.c_str());
		}
	}

	void LoggerGod::putError(const std::string& text) {
		for (auto ch : m_channels) {
			ch->error(text.c_str());
		}

		for (auto ch : this->m_privateChannels) {
			ch->error(text.c_str());
		}
	}

	void LoggerGod::putWarn(const std::string& text) {
		for (auto ch : m_channels) {
			ch->warn(text.c_str());
		}

		for (auto ch : this->m_privateChannels) {
			ch->warn(text.c_str());
		}
	}

	void LoggerGod::putInfo(const std::string& text) {
		for (auto ch : m_channels) {
			ch->info(text.c_str());
		}

		for (auto ch : this->m_privateChannels) {
			ch->info(text.c_str());
		}
	}

	void LoggerGod::putDebug(const std::string& text) {
		for (auto ch : m_channels) {
			ch->debug(text.c_str());
		}

		for (auto ch : this->m_privateChannels) {
			ch->debug(text.c_str());
		}
	}

	void LoggerGod::putTrace(const std::string& text) {
		for (auto ch : m_channels) {
			ch->verbose(text.c_str());
		}

		for (auto ch : this->m_privateChannels) {
			ch->verbose(text.c_str());
		}
	}

}