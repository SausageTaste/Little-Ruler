#pragma once

#include <string>
#include <vector>
#include <memory>


namespace dal {

	class ILoggingChannel {

	public:
		virtual ~ILoggingChannel(void) = default;

		virtual void verbose(const char* const) = 0;
		virtual void debug(const char* const) = 0;
		virtual void info(const char* const) = 0;
		virtual void warn(const char* const) = 0;
		virtual void error(const char* const) = 0;
		virtual void fatal(const char* const) = 0;

	};


	class LoggerGod {

	private:
		std::vector<ILoggingChannel*> m_channels;
		std::vector<ILoggingChannel*> m_privateChannels;

	public:
		LoggerGod(void);
		~LoggerGod(void);

		static LoggerGod& getinst(void);

		void addChannel(ILoggingChannel* const ch);
		void giveChannel(ILoggingChannel* const ch);

		void putTrace(const std::string& text);
		void putDebug(const std::string& text);
		void putInfo( const std::string& text);
		void putWarn( const std::string& text);
		void putError(const std::string& text);
		void putFatal(const std::string& text);
		
	};

}