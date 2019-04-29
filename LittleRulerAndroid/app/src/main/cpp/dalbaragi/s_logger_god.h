#pragma once

#include <string>


namespace dal {

	class LoggerGod {

	public:
		LoggerGod(void);

		static LoggerGod& getinst(void) {
			static LoggerGod inst;
			return inst;
		}

		void putFatal(const std::string& text);
		void putError(const std::string& text);
		void putWarn(const std::string& text);
		void putInfo(const std::string& text);
		void putDebug(const std::string& text);
		void putTrace(const std::string& text);

		void abort(const std::string& text);

	};

}