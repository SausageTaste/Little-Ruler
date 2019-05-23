#include "x_mainloop.h"

#include <string>
#include <vector>
#include <memory>
#include <time.h>

#include <fmt/format.h>

#include "p_dalopengl.h"
#include "s_logger_god.h"
#include "u_fileclass.h"
#include "s_configs.h"
#include "s_threader.h"


using namespace std::string_literals;


namespace {

	class FileLoggingChannel : public dal::ILoggingChannel {

	public:
		FileLoggingChannel(void) {
			dal::LoggerGod::getinst().addChannel(this);
		}

		~FileLoggingChannel(void) {
			dal::LoggerGod::getinst().deleteChannel(this);
		}

		virtual void verbose(const char* const str, const int line, const char* const func, const char* const file) override {
			
		}

		virtual void debug(const char* const str, const int line, const char* const func, const char* const file) override {

		}

		virtual void info(const char* const str, const int line, const char* const func, const char* const file) override {

		}

		virtual void warn(const char* const str, const int line, const char* const func, const char* const file) override {
			this->saveToFile("warning", str, line, func, file);
		}

		virtual void error(const char* const str, const int line, const char* const func, const char* const file) override {
			this->saveToFile("error", str, line, func, file);
		}

		virtual void fatal(const char* const str, const int line, const char* const func, const char* const file) override {
			this->saveToFile("fatal", str, line, func, file);
		}

	private:
		static void saveToFile(const char* const logLevel, const char* const str, const int line, const char* const func, const char* const file) {
			dal::LoggerGod::getinst().disable();

			const auto theTime = time(nullptr);
			struct std::tm timeInfo;

#if defined(_WIN32)
			const auto err = localtime_s(&timeInfo, &theTime);
#elif defined(__ANDROID__)
			const auto err = localtime_r(&theTime, &timeInfo);
#endif

			const auto day = timeInfo.tm_mday;
			const auto month = timeInfo.tm_mon + 1; // Month is 0 – 11, add 1 to get a jan-dec 1-12 concept
			const auto year = timeInfo.tm_year + 1900; // Year is # years since 1900
			const auto hour = timeInfo.tm_hour;
			const auto min = timeInfo.tm_min;
			const auto sec = timeInfo.tm_sec;
			
			std::string buffer{ "Dalbaragi Log\n" };
			buffer += fmt::format("{}-{}-{} {}:{}:{}\n\n", year, month, day, hour, min, sec);

			buffer += fmt::format("File : {}\nLine : {}\nFunction : {}\n", file, line, func);
			buffer += "Log level : ";
			buffer += logLevel;
			buffer += "\n\n";

			buffer += "\"\"\"\n";
			buffer += str;
			buffer += "\n\"\"\"";
			buffer += "\n\n##############\n\n";

			const auto fileID = fmt::format("log::log_{}-{}-{}_{}-{}.txt", year, month, day, hour, min);

			auto logFile = dal::resopen(fileID, dal::FileMode::append);
			if (nullptr == logFile) {
				fmt::print("Failed to create log file: {}\n", fileID);
				dal::LoggerGod::getinst().enable();
				return;
			}

			const auto res = logFile->write(buffer.c_str());
			if (!res) {
				fmt::print("Failed to write to log file: {}\n", fileID);
				dal::LoggerGod::getinst().enable();
				return;
			}

			dal::LoggerGod::getinst().enable();
		}

	} g_fileLogger;

}


namespace dal {

	// Static

	void Mainloop::giveScreenResFirst(unsigned int w, unsigned int h) {
		EventStatic e;
		e.intArg1 = w;
		e.intArg2 = h;
		e.type = EventType::window_resize;

		ConfigsGod::getinst();  // Init to make sure it registered to EventGod
		EventGod::getinst().notifyAll(e);
	}

	bool Mainloop::isScreenResGiven(void) {
		auto& query = ConfigsGod::getinst();

		auto width = query.getWinWidth();
		if (width == 0) return false;

		auto height = query.getWinHeight();
		if (height == 0) return false;

		return true;
	}

	void Mainloop::giveWhatFilesystemWants(void* androidAssetManager, const char* const sdcardPath) {
		initFilesystem(androidAssetManager, sdcardPath);
	}

	bool Mainloop::isWhatFilesystemWantsGiven(void) {
		return isFilesystemReady();
	}

	// Public

	Mainloop::Mainloop(PersistState* savedState)
	:	m_flagQuit(false),
		m_inputApply(m_renderMan.m_overlayMas)
	{
		// Check window res
		{
			auto& query = ConfigsGod::getinst();
			auto width = query.getWinWidth();
			auto height = query.getWinHeight();
			if (width == 0 || height == 0) {
				dalAbort("Please call Mainloop::giveScreenResFirst before constructor!");
			}
			this->onResize(width, height);
		}

		// Check filesystem init
		{
			if (!isWhatFilesystemWantsGiven()) {
				dalAbort("Please call Mainloop::giveWhatFilesystemWants before constructor!");
			}
		}

		// Misc
		{
			mHandlerName = "dal::Mainloop";
			EventGod::getinst().registerHandler(this, EventType::quit_game);
			
			this->m_timer.setCapFPS(300);
		}

		// Restore from saved state
		{
			if (savedState != nullptr) {
				m_renderMan.mCameraPos = savedState->cameraPos;
				m_renderMan.mCameraViewDir = savedState->cameraViewDir;
				delete savedState;
			}
		}

		// Test
		{

		}

		const auto elapsed = m_initTimer.check_getElapsed_capFPS();
		dalInfo("Init time: "s + std::to_string(elapsed));
	}

	Mainloop::~Mainloop(void) {
		EventGod::getinst().deregisterHandler(this, EventType::quit_game);
	}

	int Mainloop::update(void) {
		if (m_flagQuit) return -1;

		const auto deltaTime = m_timer.check_getElapsed_capFPS();
		if (m_timerForFPSReport.hasElapsed(0.1f)) {
			m_renderMan.m_overlayMas.setDisplayedFPS((unsigned int)(1.0f / deltaTime));
			m_timerForFPSReport.check();
		}

		m_inputApply.apply(deltaTime, &m_renderMan.mCameraPos, &m_renderMan.mCameraViewDir);

		TaskGod::getinst().update();

		this->m_renderMan.update(deltaTime);
		this->m_renderMan.render();

		return 0;
	}

	void Mainloop::onResize(int width, int height) {
		EventStatic e;
		e.intArg1 = width;
		e.intArg2 = height;
		e.type = EventType::window_resize;
		EventGod::getinst().notifyAll(e);
	}

	PersistState* Mainloop::getSavedState(void) {
		auto s = new PersistState();
		s->cameraPos = m_renderMan.mCameraPos;
		s->cameraViewDir = m_renderMan.mCameraViewDir;
		return s;
	}

	void Mainloop::onEvent(const EventStatic& e) {
		switch (e.type) {

		case EventType::quit_game:
			this->m_flagQuit = true;
			break;
		default:
			dalWarn("dal::Mainloop can't handle this event:");

		}
	}

}