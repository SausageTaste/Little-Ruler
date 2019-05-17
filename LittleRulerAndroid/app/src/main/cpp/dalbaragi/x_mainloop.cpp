#include "x_mainloop.h"

#include <string>
#include <vector>
#include <memory>

#include <fmt/format.h>
#define ZLIB_WINAPI
#include <zlib.h>

#include "p_dalopengl.h"
#include "s_logger_god.h"
#include "u_fileclass.h"
#include "s_configs.h"
#include "s_threader.h"


using namespace std::string_literals;


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

	void Mainloop::giveWhatFilesystemWants(void* androidAssetManager) {
		filec::initFilesystem(androidAssetManager);
	}

	bool Mainloop::isWhatFilesystemWantsGiven(void) {
		return filec::isFilesystemReady();
	}

	// Public

	Mainloop::Mainloop(PersistState* savedState)
	:	m_flagQuit(false),
		m_inputApply(m_renderMan.m_overlayMas)
	{
		/* Check window res */ {
			auto& query = ConfigsGod::getinst();
			auto width = query.getWinWidth();
			auto height = query.getWinHeight();
			if (width == 0 || height == 0) {
				LoggerGod::getinst().putFatal("Please call Mainloop::giveScreenResFirst before constructor!");
				throw -1;
			}
			this->onResize(width, height);
		}

		/* Check filesystem init */ {
			if (!isWhatFilesystemWantsGiven()) {
				LoggerGod::getinst().putFatal("Please call Mainloop::giveWhatFilesystemWants before constructor!");
				throw -1;
			}
		}

		/* Misc */ {
			mHandlerName = "dal::Mainloop";
			EventGod::getinst().registerHandler(this, EventType::quit_game);
			
			this->m_timer.setCapFPS(300);
		}

		/* Restore from saved state */ {
			if (savedState != nullptr) {
				m_renderMan.mCameraPos = savedState->cameraPos;
				m_renderMan.mCameraViewDir = savedState->cameraViewDir;
				delete savedState;
			}
		}

		// Test
		{
			std::vector<uint8_t> buffer;
			const auto res = filec::getResource_buffer("asset::test.tst", buffer);
			assert(res);
			uLongf bufSize = 1024;
			uint8_t buf[1024];

			const auto zipres = uncompress(buf, &bufSize, buffer.data(), buffer.size());
			buf[bufSize] = 0;
			const auto str = reinterpret_cast<char*>(buf);
			LoggerGod::getinst().putInfo("good: "s + str);
		}

		const auto elapsed = m_initTimer.check_getElapsed_capFPS();
		LoggerGod::getinst().putInfo("Init time: "s + std::to_string(elapsed));
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
			LoggerGod::getinst().putWarn("dal::Mainloop can't handle this event:");

		}
	}

}