#include "o_overlay_master.h"

#include <string>

#include "u_fileclass.h"
#include "s_logger_god.h"
#include "p_glglobal.h"
#include "s_freetype_master.h"
#include "p_globalfsm.h"


using namespace std::string_literals;


namespace dal {

	OverlayMaster::OverlayMaster(TextureMaster& texMaster, ResourceMaster& resMas)
		: m_resMas(resMas),
		m_asciiCache(texMaster, resMas),
		mGlobalFSM(GlobalFSM::game), mShaderOverlay("OverlayMaster::overlay")
	{
		/* Compile shaders overlay */ {
			std::string vertSrc, fragSrc;
			filec::getAsset_text("glsl/overlay_v.glsl", &vertSrc);
			filec::getAsset_text("glsl/overlay_f.glsl", &fragSrc);

			auto verShader = compileShader(ShaderType::VERTEX, vertSrc.c_str());
			auto fragShader = compileShader(ShaderType::FRAGMENT, fragSrc.c_str());

			this->mShaderOverlay.attachShader(verShader);
			this->mShaderOverlay.attachShader(fragShader);
			this->mShaderOverlay.link();
			this->mUnilocOverlay.init(this->mShaderOverlay);

			glDeleteShader(verShader);
			glDeleteShader(fragShader);
		}

		/* Characters */ {
			mDisplayFPS.setPos(10.0f, 10.0f);
			mDisplayFPS.setSize(100.0f, 20.0f);

			mLineEdit.setPos(10.0f, 40.0f);
			mLineEdit.setSize(500, 20.0f);
			mLineEdit.setText("Hello world!");
		}

		/* Event Master */ {
			this->mHandlerName = "OverlayMaster";
			EventGod::getinst().registerHandler(this, EventType::window_resize);
			EventGod::getinst().registerHandler(this, EventType::touch_tap);
			EventGod::getinst().registerHandler(this, EventType::global_fsm_change);
		}

		/* Misc */ {
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			for (unsigned int i = 0; i < 10; i++) {
				mBoxesForTouchPoint.at(i).moveCenterTo_screenCoord(-100.0f, -100.0f);
			}

			mBoxesForTouchPoint[0].setColor(1.0f, 0.0f, 0.0f);
			mBoxesForTouchPoint[1].setColor(0.0f, 1.0f, 0.0f);
			mBoxesForTouchPoint[2].setColor(0.0f, 0.0f, 1.0f);
			mBoxesForTouchPoint[10].setColor(1.0f, 1.0f, 0.0f);
			mBoxesForTouchPoint[10].setTransparency(0.5f);

			mDebugPlane.setWidth(200);
			mDebugPlane.setHeight(200);
			mDebugPlane.moveCornerTo_screenCoord(10, 50);
		}
	}

	OverlayMaster::~OverlayMaster(void) {
		EventGod::getinst().deregisterHandler(this, EventType::window_resize);
		EventGod::getinst().deregisterHandler(this, EventType::touch_tap);
		EventGod::getinst().deregisterHandler(this, EventType::global_fsm_change);
	}

	void OverlayMaster::onEvent(const EventStatic& e) {
		switch (e.type) {

		case EventType::window_resize:
			mDisplayFPS.onResize();
			mLineEdit.onResize();
			break;
		case EventType::touch_tap:
			if (mGlobalFSM == GlobalFSM::menu && mLineEdit.isInside({ e.floatArg1, e.floatArg2 })) {
				LoggerGod::getinst().putInfo("Tap on LineEdit.");

				EventStatic newEvent;
				newEvent.type = EventType::global_fsm_change;
				newEvent.intArg1 = int(GlobalFSM::menu);
				newEvent.keyListner = &mLineEdit;
				EventGod::getinst().notifyAll(newEvent);
			}
			break;
		case EventType::global_fsm_change:
			mGlobalFSM = GlobalFSM(e.intArg1);
			break;
		default:
			LoggerGod::getinst().putWarn("Unhanlded event in OverlayMaster.");
			break;

		}
	}

	void OverlayMaster::render(void) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		GLSwitch::setFor_overlay();

		mShaderOverlay.use();

		for (unsigned int i = 0; i < 11; i++) {
			mBoxesForTouchPoint.at(i).renderOverlay(mUnilocOverlay);
		}

		mDisplayFPS.renderOverlay(m_asciiCache, mUnilocOverlay);

		if (mGlobalFSM == GlobalFSM::menu) {
			mLineEdit.renderOverlay(m_asciiCache, mUnilocOverlay);
			mDebugPlane.renderOverlay(mUnilocOverlay);
		}
	}

	void OverlayMaster::setDisplayedFPS(unsigned int fps) {
		mDisplayFPS.setText(std::to_string(fps).c_str());
	}

}