#include "o_overlay_master.h"

#include <string>

#include "u_fileclass.h"
#include "s_logger_god.h"
#include "s_freetype_master.h"
#include "s_configs.h"


using namespace std::string_literals;


namespace dal {

    OverlayMaster::TextStreamChannel::TextStreamChannel(dal::TextStream& texStream)
        : m_texStream(texStream)
    {

    }

    void OverlayMaster::TextStreamChannel::verbose(const char* const str, const int line, const char* const func, const char* const file) {
        const auto text = "[VERBO] "s + str + '\n';
        this->m_texStream.append(text);
    }

    void OverlayMaster::TextStreamChannel::debug(const char* const str, const int line, const char* const func, const char* const file) {
        const auto text = "[DEBUG] "s + str + '\n';
        this->m_texStream.append(text);
    }

    void OverlayMaster::TextStreamChannel::info(const char* const str, const int line, const char* const func, const char* const file) {
        const auto text = "[INFO] "s + str + '\n';
        this->m_texStream.append(text);
    }

    void OverlayMaster::TextStreamChannel::warn(const char* const str, const int line, const char* const func, const char* const file) {
        const auto text = "[WARN] "s + str + '\n';
        this->m_texStream.append(text);
    }

    void OverlayMaster::TextStreamChannel::error(const char* const str, const int line, const char* const func, const char* const file) {
        const auto text = "[ERROR] "s + str + '\n';
        this->m_texStream.append(text);
    }

    void OverlayMaster::TextStreamChannel::fatal(const char* const str, const int line, const char* const func, const char* const file) {
        const auto text = "[FATAL] "s + str + '\n';
        this->m_texStream.append(text);
    }

}


namespace dal {

    OverlayMaster::OverlayMaster(ResourceMaster& resMas, const ShaderMaster& shaderMas, const unsigned int width, const unsigned int height)
        : m_resMas(resMas)
        , m_shaderMas(shaderMas)
        , m_unicodes(resMas)
        , mGlobalFSM(GlobalGameState::game)
        , m_texStreamCh(m_strBuffer)
    {
        ConfigsGod::getinst().setWinSize(width, height);
        script::set_outputStream(&this->m_strBuffer);

        // Widgets
        {
            {
                auto fpsDisplayer = new Label(nullptr, this->m_unicodes);

                fpsDisplayer->setPosX(10.0f);
                fpsDisplayer->setPosY(10.0f);
                fpsDisplayer->setWidth(50.0f);
                fpsDisplayer->setHeight(20.0f);
                fpsDisplayer->setPauseOnly(false);
                fpsDisplayer->setBackgroundColor(0.0f, 0.0f, 0.0f, 0.5f);

                this->mDisplayFPS = fpsDisplayer;
                this->giveWidgetOwnership(fpsDisplayer);
            }

            {
                auto wid = new LineEdit(nullptr, this->m_unicodes);

                wid->setPosX(10.0f);
                wid->setPosY(10.0f);
                wid->setWidth(800.0f);
                wid->setHeight(20.0f);
                wid->setAlignMode(ScreenQuad::AlignMode::upper_right);

                this->giveWidgetOwnership(wid);
            }

            {
                auto wid = new TextBox(nullptr, this->m_unicodes);

                this->m_strBuffer.append("Sungmin Woo\nwoos8899@gmail.com\n\n");
                wid->setStrBuf(&this->m_strBuffer);

                wid->setPosX(10.0f);
                wid->setPosY(40.0f);
                wid->setWidth(800.0f);
                wid->setHeight(600.0f);
                wid->setAlignMode(ScreenQuad::AlignMode::upper_right);

                this->giveWidgetOwnership(wid);
            }
        }

        // Event Master
        {
            this->mHandlerName = "OverlayMaster";
            EventGod::getinst().registerHandler(this, EventType::global_fsm_change);
        }

        // Misc
        {
            LoggerGod::getinst().addChannel(&m_texStreamCh);
        }
    }

    OverlayMaster::~OverlayMaster(void) {
        LoggerGod::getinst().deleteChannel(&m_texStreamCh);
        EventGod::getinst().deregisterHandler(this, EventType::global_fsm_change);
        this->mDisplayFPS = nullptr;
        this->m_widgets.clear();
    }

    void OverlayMaster::onEvent(const EventStatic& e) {
        if ( EventType::global_fsm_change == e.type ) {
            mGlobalFSM = GlobalGameState(e.intArg1);
        }
        else {
            LoggerGod::getinst().putWarn("Unhanlded event in OverlayMaster.", __LINE__, __func__, __FILE__);
        }
    }

    void OverlayMaster::onWinResize(const unsigned int width, const unsigned int height) {
        for ( auto& wid : this->m_widgets ) {
            wid->onResize(width, height);
        }
    }

    void OverlayMaster::onClick(const float x, const float y) {
        if ( GlobalGameState::game == this->mGlobalFSM ) {
            for ( auto wid : this->m_widgets ) {
                if ( wid->getPauseOnly() ) continue;
                if ( !wid->isInside(x, y) ) continue;

                wid->onClick(x, y);
                break;
            }
        }
        else {
            for ( auto wid : this->m_widgets ) {
                if ( wid->isInside(x, y) ) {
                    this->m_widgets.remove(wid);
                    this->m_widgets.push_front(wid);

                    wid->onClick(x, y);
                    wid->onFocusChange(true);
                    break;
                }
            }

            auto iter = this->m_widgets.begin();
            while ( ++iter != this->m_widgets.end() ) {
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
        if ( this->m_widgets.empty() ) return;

        this->m_widgets.front()->onKeyInput(str.c_str());
    }

    void OverlayMaster::render(void) const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        auto& uniloc = this->m_shaderMas.useOverlay();

        const auto end = this->m_widgets.rend();
        for ( auto iter = this->m_widgets.rbegin(); iter != end; ++iter ) {
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

    void OverlayMaster::giveWidgetOwnership(Widget* const w) {
        for ( auto widget : this->m_widgets ) {
            widget->onFocusChange(false);
        }

        this->m_widgets.push_front(w);
        this->m_toDelete.push_back(w);

        w->onFocusChange(true);
    }

    void OverlayMaster::giveWidgetRef(Widget* const w) {
        for ( auto widget : this->m_widgets ) {
            widget->onFocusChange(false);
        }

        this->m_widgets.push_front(w);

        w->onFocusChange(true);
    }

    void OverlayMaster::setDisplayedFPS(const unsigned int fps) {
        this->mDisplayFPS->setText(std::to_string(fps));
    }

    // Private

    void OverlayMaster::setFocusOn(Widget* const w) {
        this->m_widgets.remove(w);
        for ( auto widget : this->m_widgets ) {
            widget->onFocusChange(false);
        }
        this->m_widgets.push_front(w);
    }

}