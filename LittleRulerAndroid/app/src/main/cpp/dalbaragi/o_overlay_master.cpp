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
	:	m_resMas(resMas),
		m_shaderMas(shaderMas),
		m_unicodes(resMas),
		mGlobalFSM(GlobalFSM::game)
	{
		/* Characters */ {
			script::set_outputStream(&this->m_strBuffer);

			{
				auto fpsDisplayer = new LineEdit(this->m_unicodes);
				
				fpsDisplayer->setPosX(10.0f);
				fpsDisplayer->setPosY(10.0f);
				fpsDisplayer->setWidth(100.0f);
				fpsDisplayer->setHeight(20.0f);
				fpsDisplayer->setPauseOnly(false);
				fpsDisplayer->setBoxColor(0.0f, 0.0f, 0.0f, 0.0f);

				this->mDisplayFPS = fpsDisplayer;
				this->m_widgets.push_back(fpsDisplayer);
			}

			{
				auto wid = new LineEdit(this->m_unicodes);

				wid->setPosX(10.0f);
				wid->setPosY(10.0f);
				wid->setWidth(400.0f);
				wid->setHeight(20.0f);
				wid->setAlignMode(ScreenQuad::AlignMode::upper_right);

				this->m_widgets.push_back(wid);
			}

			{
				auto wid = new TextBox(this->m_unicodes);

				this->m_strBuffer.append(u8"¿ì¼º¹Î\nSo«»ÁöÚ«\nwoos8899@gmail.com\n\n");

				wid->setStrBuf(&this->m_strBuffer);
				wid->setPosX(10.0f);
				wid->setPosY(40.0f);
				wid->setWidth(400.0f);
				wid->setHeight(300.0f);
				wid->setAlignMode(ScreenQuad::AlignMode::upper_right);

				this->m_widgets.push_back(wid);
			}
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

		this->mDisplayFPS = nullptr;

		for (auto win : this->m_widgets) {
			delete win;
		}
		this->m_widgets.clear();
	}

	void OverlayMaster::onEvent(const EventStatic& e) {
		if (EventType::window_resize == e.type) {
			const auto width = ConfigsGod::getinst().getWinWidth();
			const auto height = ConfigsGod::getinst().getWinHeight();

			for (auto wid : this->m_widgets) {
				wid->onResize(width, height);
			}
		}
		else if (EventType::global_fsm_change == e.type) {
			mGlobalFSM = GlobalFSM(e.intArg1);
		}
		else {
			LoggerGod::getinst().putWarn("Unhanlded event in OverlayMaster.");
		}
	}

	void OverlayMaster::onClick(const float x, const float y) {
		g_logger.putTrace("Click: "s + std::to_string(x) + ", " + std::to_string(y));

		if (GlobalFSM::menu != this->mGlobalFSM) return;

		for (auto wid : this->m_widgets) {
			if (wid->isInside(x, y)) {
				this->m_widgets.remove(wid);
				this->m_widgets.push_front(wid);
				
				wid->onClick(x, y);
				wid->onFocusChange(true);
				break;
			}
		}

		bool skippedOnce = false;
		for (auto wid : this->m_widgets) {
			if (!skippedOnce) continue;
			wid->onFocusChange(false);
		}
	}

	void OverlayMaster::onDrag(const glm::vec2& start, const glm::vec2& end) {
		g_logger.putTrace(
			"Drag: "s + std::to_string(start.x) + ", " + std::to_string(start.y) + " -> " + std::to_string(end.x) + ", " + std::to_string(end.y)
		);
	}

	void OverlayMaster::onKeyInput(const std::string& str) {
		if (this->m_widgets.empty()) return;

		for (const auto c : str) {
			this->m_widgets.front()->onKeyInput(c);
		}
	}

	void OverlayMaster::render(void) const {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		GLSwitch::setFor_overlay();

		this->m_shaderMas.useOverlay();
		auto& uniloc = this->m_shaderMas.getOverlay();

		for (unsigned int i = 0; i < 11; i++) {
			this->mBoxesForTouchPoint.at(i).renderOverlay(uniloc);
		}

		for (auto wid : this->m_widgets) {
			if (wid->getPauseOnly()) {
				if (GlobalFSM::menu == mGlobalFSM) {
					wid->renderOverlay(uniloc);
				}
			}
			else {
				wid->renderOverlay(uniloc);
			}
		}
	}

	void OverlayMaster::setDisplayedFPS(const unsigned int fps) {
		this->mDisplayFPS->setText(std::to_string(fps));
	}

}