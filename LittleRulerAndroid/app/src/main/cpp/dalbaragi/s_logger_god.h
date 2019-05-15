#pragma once

#include <string>
#include <vector>


namespace dal {

	class ILoggerChannel {

	public:
		virtual ~ILoggerChannel(void) = default;
		virtual void plane(const char* const) = 0;
		virtual void error(const char* const) = 0;

	};


	class LoggerGod {

	private:
		std::vector<ILoggerChannel*> m_channels;

	public:
		static LoggerGod& getinst(void);

		void addChannel(ILoggerChannel* ch);

		void putTrace(const std::string& text);
		void putDebug(const std::string& text);
		void putInfo( const std::string& text);
		void putWarn( const std::string& text);
		void putError(const std::string& text);
		void putFatal(const std::string& text);
		
	};

}