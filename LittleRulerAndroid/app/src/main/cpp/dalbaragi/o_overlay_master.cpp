#include "o_overlay_master.h"

#include <string>

#include "u_fileclass.h"
#include "s_logger_god.h"
#include "s_freetype_master.h"
#include "s_configs.h"


using namespace std::string_literals;


namespace {

	auto& g_logger = dal::LoggerGod::getinst();

}


namespace dal {

	OverlayMaster::TextStreamChannel::TextStreamChannel(dal::TextStream& texStream)
		: m_texStream(texStream)
	{

	}

	void OverlayMaster::TextStreamChannel::verbose(const char* const str, const int line, const char* const func, const char* const file) {
		const auto text = "[VERBO]"s + str + '\n';
		this->m_texStream.append(text);
	}

	void OverlayMaster::TextStreamChannel::debug(const char* const str, const int line, const char* const func, const char* const file) {
		const auto text = "[DEBUG]"s + str + '\n';
		this->m_texStream.append(text);
	}

	void OverlayMaster::TextStreamChannel::info(const char* const str, const int line, const char* const func, const char* const file) {
		const auto text = "[INFO]"s + str + '\n';
		this->m_texStream.append(text);
	}

	void OverlayMaster::TextStreamChannel::warn(const char* const str, const int line, const char* const func, const char* const file) {
		const auto text = "[WARN]"s + str + '\n';
		this->m_texStream.append(text);
	}

	void OverlayMaster::TextStreamChannel::error(const char* const str, const int line, const char* const func, const char* const file) {
		const auto text = "[ERROR]"s + str + '\n';
		this->m_texStream.append(text);
	}

	void OverlayMaster::TextStreamChannel::fatal(const char* const str, const int line, const char* const func, const char* const file) {
		const auto text = "[FATAL]"s + str + '\n';
		this->m_texStream.append(text);
	}

}


namespace dal {

	OverlayMaster::OverlayMaster(ResourceMaster& resMas, const ShaderMaster& shaderMas)
	:	m_resMas(resMas),
		m_shaderMas(shaderMas),
		m_unicodes(resMas),
		mGlobalFSM(GlobalGameState::game),
		m_texStreamCh(m_strBuffer)
	{
		/* Characters */ {
			script::set_outputStream(&this->m_strBuffer);

			{
				auto fpsDisplayer = new Label(nullptr, this->m_unicodes);
				
				fpsDisplayer->setPosX(10.0f);
				fpsDisplayer->setPosY(10.0f);
				fpsDisplayer->setWidth(50.0f);
				fpsDisplayer->setHeight(20.0f);
				fpsDisplayer->setPauseOnly(false);
				fpsDisplayer->setBackgroundColor(0.0f, 0.0f, 0.0f, 0.5f);

				this->mDisplayFPS = fpsDisplayer;
				this->m_widgets.push_back(fpsDisplayer);
			}

			{
				auto wid = new LineEdit(nullptr, this->m_unicodes);

				wid->setPosX(10.0f);
				wid->setPosY(10.0f);
				wid->setWidth(800.0f);
				wid->setHeight(20.0f);
				wid->setAlignMode(ScreenQuad::AlignMode::upper_right);

				this->m_widgets.push_back(wid);
			}

			{
				auto wid = new TextBox(nullptr, this->m_unicodes);
				this->m_strBuffer.append("Sungmin Woo\n우성민\nwoos8899@gmail.com\n\n");
				wid->setStrBuf(&this->m_strBuffer);

				wid->setPosX(10.0f);
				wid->setPosY(40.0f);
				wid->setWidth(800.0f);
				wid->setHeight(600.0f);
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

			g_logger.addChannel(&m_texStreamCh);
		}
	}

	OverlayMaster::~OverlayMaster(void) {
		g_logger.deleteChannel(&m_texStreamCh);

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
			mGlobalFSM = GlobalGameState(e.intArg1);
		}
		else {
			LoggerGod::getinst().putWarn("Unhanlded event in OverlayMaster.", __LINE__, __func__, __FILE__);
		}
	}

	void OverlayMaster::onClick(const float x, const float y) {
		if (GlobalGameState::game == this->mGlobalFSM) {
			for (auto wid : this->m_widgets) {
				if (wid->getPauseOnly() && !wid->isInside(x, y)) continue;
				wid->onClick(x, y);
				break;
			}
		}
		else {
			for (auto wid : this->m_widgets) {
				if (wid->isInside(x, y)) {
					this->m_widgets.remove(wid);
					this->m_widgets.push_front(wid);

					wid->onClick(x, y);
					wid->onFocusChange(true);
					break;
				}
			}

			auto iter = this->m_widgets.begin();
			while (++iter != this->m_widgets.end()) {
				(*iter)->onFocusChange(false);
			}
		}
	}

	void OverlayMaster::onDrag(const glm::vec2& start, const glm::vec2& end) {
		//g_logger.putTrace(
		//	"Drag: "s + std::to_string(start.x) + ", " + std::to_string(start.y) + " -> " + std::to_string(end.x) + ", " + std::to_string(end.y)
		//);
	}

	void OverlayMaster::onKeyInput(const std::string& str) {
		if (this->m_widgets.empty()) return;

		this->m_widgets.front()->onKeyInput(str.c_str());
	}

	void OverlayMaster::render(void) const {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		this->m_shaderMas.useOverlay();
		auto& uniloc = this->m_shaderMas.getOverlay();

		for (unsigned int i = 0; i < 11; i++) {
			this->mBoxesForTouchPoint.at(i).renderOverlay(uniloc);
		}

		for ( auto iter = this->m_widgets.rbegin(); iter != this->m_widgets.rend(); ++iter ) {
			if ( (*iter)->getPauseOnly() ) {
				if ( GlobalGameState::menu == mGlobalFSM ) {
					(*iter)->renderOverlay(uniloc);
				}
			}
			else {
				(*iter)->renderOverlay(uniloc);
			}
		}
	}

	void OverlayMaster::addWidget(Widget* const w) {
		this->m_widgets.emplace_front(w);
	}

	void OverlayMaster::setDisplayedFPS(const unsigned int fps) {
		this->mDisplayFPS->setText(std::to_string(fps));
	}

}