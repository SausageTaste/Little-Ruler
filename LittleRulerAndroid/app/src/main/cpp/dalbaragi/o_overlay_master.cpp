#include "o_overlay_master.h"

#include <string>

#include "u_fileclass.h"
#include "s_logger_god.h"
#include "p_glglobal.h"
#include "s_freetype_master.h"


using namespace std::string_literals;


namespace dal {

	OverlayMaster::OverlayMaster(ResourceMaster& resMas, const ShaderMaster& shaderMas)
		: m_resMas(resMas),
		m_shaderMas(shaderMas),
		m_asciiCache(resMas),
		mGlobalFSM(GlobalFSM::game),
		m_keyTaker(nullptr)
	{
		/* Characters */ {
			mDisplayFPS.setPos(10.0f, 10.0f);
			mDisplayFPS.setSize(100.0f, 20.0f);

			mLineEdit.setPos(10.0f, 40.0f);
			mLineEdit.setSize(500.0f, 20.0f);
			mLineEdit.setText("print(\"Hello world!\")");
		}

		/* Event Master */ {
			this->mHandlerName = "OverlayMaster";
			EventGod::getinst().registerHandler(this, EventType::window_resize);
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
		}
	}

	OverlayMaster::~OverlayMaster(void) {
		EventGod::getinst().deregisterHandler(this, EventType::window_resize);
		EventGod::getinst().deregisterHandler(this, EventType::global_fsm_change);
	}

	void OverlayMaster::onEvent(const EventStatic& e) {
		switch (e.type) {

		case EventType::window_resize:
			mDisplayFPS.onResize();
			mLineEdit.onResize();
			break;
		case EventType::global_fsm_change:
			mGlobalFSM = GlobalFSM(e.intArg1);
			mLineEdit.setTextColor(0.4f, 0.4f, 0.4f);
			break;
		default:
			LoggerGod::getinst().putWarn("Unhanlded event in OverlayMaster.");
			break;

		}
	}

	void OverlayMaster::onClick(const float x, const float y) {
		if (mGlobalFSM == GlobalFSM::menu && mLineEdit.isInside({ x, y })) {
			this->m_keyTaker = &this->mLineEdit;
			mLineEdit.setTextColor(1.0f, 1.0f, 1.0f);
		}
		else {
			this->m_keyTaker = nullptr;
			mLineEdit.setTextColor(0.4f, 0.4f, 0.4f);
		}
	}

	void OverlayMaster::onKeyInput(const std::string& str) {
		if (nullptr == this->m_keyTaker) return;

		for (const auto c : str) {
			this->m_keyTaker->onKeyInput(c);
		}
	}

	void OverlayMaster::render(void) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		GLSwitch::setFor_overlay();

		this->m_shaderMas.useOverlay();
		auto& uniloc = this->m_shaderMas.getOverlay();

		for (unsigned int i = 0; i < 11; i++) {
			mBoxesForTouchPoint.at(i).renderOverlay(uniloc);
		}

		mDisplayFPS.renderOverlay(m_asciiCache, uniloc);

		if (mGlobalFSM == GlobalFSM::menu) {
			mLineEdit.renderOverlay(m_asciiCache, uniloc);
		}
	}

	void OverlayMaster::setDisplayedFPS(unsigned int fps) {
		mDisplayFPS.setText(std::to_string(fps).c_str());
	}

}