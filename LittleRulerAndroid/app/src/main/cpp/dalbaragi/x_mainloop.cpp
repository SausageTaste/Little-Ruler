#include "x_mainloop.h"

#include <string>
#include <vector>
#include <memory>

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
	:	mFlagQuit(false),
		mInputApply(mRenderMan.m_overlayMas.mBoxesForTouchPoint)
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
			
			this->mTimer.setCapFPS(300);
		}

		/* Restore from saved state */ {
			if (savedState != nullptr) {
				mRenderMan.mCameraPos = savedState->cameraPos;
				mRenderMan.mCameraViewDir = savedState->cameraViewDir;
				delete savedState;
			}
		}

		// Test
		{

		}

		const auto elapsed = initTimer.check_getElapsed_capFPS();
		LoggerGod::getinst().putInfo("Init time: "s + std::to_string(elapsed));
	}

	Mainloop::~Mainloop(void) {
		EventGod::getinst().deregisterHandler(this, EventType::quit_game);
	}

	int Mainloop::update(void) {
		if (mFlagQuit) return -1;

		const auto deltaTime = mTimer.check_getElapsed_capFPS();
		if (mTimerForFPSReport.hasElapsed(0.1f)) {
			mRenderMan.m_overlayMas.setDisplayedFPS((unsigned int)(1.0f / deltaTime));
			mTimerForFPSReport.check();
		}

		mInputApply.apply(deltaTime, &mRenderMan.mCameraPos, &mRenderMan.mCameraViewDir);

		TaskGod::getinst().update();

		this->mRenderMan.update(deltaTime);
		this->mRenderMan.render();

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
		s->cameraPos = mRenderMan.mCameraPos;
		s->cameraViewDir = mRenderMan.mCameraViewDir;
		return s;
	}

	void Mainloop::onEvent(const EventStatic& e) {
		switch (e.type) {

		case EventType::quit_game:
			this->mFlagQuit = true;
			break;
		default:
			LoggerGod::getinst().putWarn("dal::Mainloop can't handle this event:");

		}
	}

}