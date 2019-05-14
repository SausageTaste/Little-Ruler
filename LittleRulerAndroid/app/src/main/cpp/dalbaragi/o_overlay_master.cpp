#include "o_overlay_master.h"

#include <string>

#include "u_fileclass.h"
#include "s_logger_god.h"
#include "p_glglobal.h"
#include "s_freetype_master.h"
#include "s_configs.h"


using namespace std::string_literals;


namespace {

	auto& g_logger = dal::LoggerGod::getinst();

}


namespace dal {

	OverlayMaster::OverlayMaster(ResourceMaster& resMas, const ShaderMaster& shaderMas)
		: m_resMas(resMas),
		m_shaderMas(shaderMas),
		m_asciiCache(resMas),
		mGlobalFSM(GlobalFSM::game),
		m_texBox(m_asciiCache),
		m_keyTaker(nullptr)
	{
		/* Characters */ {
			script::set_outputStream(&this->m_strBuffer);

			mDisplayFPS.setPos(10.0f, 10.0f);
			mDisplayFPS.setSize(100.0f, 20.0f);

			mLineEdit.setPos(10.0f, 40.0f);
			mLineEdit.setSize(400.0f, 20.0f);
			mLineEdit.setText("print(\"Hello world!\")");

			this->m_texBox.setStrBuf(&this->m_strBuffer);
			this->m_texBox.setPosX(10.0f);
			this->m_texBox.setPosY(70.0f);
			this->m_texBox.setWidth(400.0f);
			this->m_texBox.setHeight(300.0f);
			this->m_texBox.setAlignMode(ScreenQuad::AlignMode::upper_right);
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
		if (EventType::window_resize == e.type) {
			const auto width = ConfigsGod::getinst().getWinWidth();
			const auto height = ConfigsGod::getinst().getWinHeight();

			mDisplayFPS.onResize();
			mLineEdit.onResize();
			this->m_texBox.onResize(width, height);
		}
		else if (EventType::global_fsm_change == e.type) {
			mGlobalFSM = GlobalFSM(e.intArg1);
			mLineEdit.setTextColor(0.4f, 0.4f, 0.4f);
		}
		else {
			LoggerGod::getinst().putWarn("Unhanlded event in OverlayMaster.");
		}
	}

	void OverlayMaster::onClick(const float x, const float y) {
		g_logger.putTrace("Click: "s + std::to_string(x) + ", " + std::to_string(y));

		if (mGlobalFSM == GlobalFSM::menu && mLineEdit.isInside({ x, y })) {
			this->m_keyTaker = &this->mLineEdit;
			mLineEdit.setTextColor(1.0f, 1.0f, 1.0f);
		}
		else {
			this->m_keyTaker = nullptr;
			mLineEdit.setTextColor(0.4f, 0.4f, 0.4f);
		}
	}

	void OverlayMaster::onDrag(const glm::vec2& start, const glm::vec2& end) {
		g_logger.putTrace(
			"Drag: "s + std::to_string(start.x) + ", " + std::to_string(start.y) + " -> " + std::to_string(end.x) + ", " + std::to_string(end.y)
		);
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
			this->m_texBox.renderOverlay(uniloc);
		}
	}

	void OverlayMaster::setDisplayedFPS(unsigned int fps) {
		mDisplayFPS.setText(std::to_string(fps).c_str());
	}

}