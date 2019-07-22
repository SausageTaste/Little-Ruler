#include "o_overlay_master.h"

#include <string>
#include <unordered_map>

#include <fmt/format.h>

#include "u_fileclass.h"
#include "s_logger_god.h"
#include "s_freetype_master.h"
#include "s_configs.h"


using namespace std::string_literals;
using namespace fmt::literals;


namespace {

    class TouchStatesMaster {

    private:
        struct TouchState {
            glm::vec2 m_lastDownPos, m_pos;
            dal::Widget2* m_owner = nullptr;
            float m_lastDownSec = 0.0f, m_sec = 0.0f;
            bool m_down = false;
        };

    private:
        using states_t = std::unordered_map<dal::touchID_t, TouchState>;
        states_t m_states;

    public:
        void dispatch(const float winWidth, const float winHeight, std::list<dal::Widget2*>& widgets, dal::Widget2* const bgWidget) {
            const float widthOrHeightButShorter = winWidth < winHeight ? winWidth : winHeight;
            const float aThridWidth = winWidth / 3.0f;

            auto& tq = dal::TouchEvtQueueGod::getinst();

            for ( unsigned int i = 0; i < tq.getSize(); i++ ) {
                const auto& tevent = tq.at(i);
                auto& state = this->getOrMakeTouchState(tevent.id, this->m_states);

                if ( nullptr != state.m_owner ) {
                    const auto ctrlFlag = state.m_owner->onTouch(tevent);
                    switch ( ctrlFlag ) {

                    case dal::InputCtrlFlag::consumed:
                        state.m_owner = nullptr;
                        break;
                    case dal::InputCtrlFlag::ignored:
                        state.m_owner = nullptr;
                        goto startWidgetsLoop;
                    case dal::InputCtrlFlag::owned:
                        break;
                    default:
                        dalAbort("Shouldn't reach here, the enum's index is: {}"_format(static_cast<int>(ctrlFlag)));

                    }
                }
                else {
                startWidgetsLoop:

                    for ( auto w : widgets ) {
                        if ( !w->isPointInside(tevent.x, tevent.y) ) {
                            continue;
                        }

                        const auto ctrlFlag = w->onTouch(tevent);
                        switch ( ctrlFlag ) {

                        case dal::InputCtrlFlag::consumed:
                            goto endWidgetsLoop;
                        case dal::InputCtrlFlag::ignored:
                            continue;
                        case dal::InputCtrlFlag::owned:
                            state.m_owner = w;
                            goto endWidgetsLoop;
                        default:
                            dalAbort("Shouldn't reach here, the enum's index is: {}"_format(static_cast<int>(ctrlFlag)));

                        }
                    }

                    if ( nullptr != bgWidget && bgWidget->isPointInside(tevent.x, tevent.y) ) {
                        const auto ctrlFlag = bgWidget->onTouch(tevent);
                        switch ( ctrlFlag ) {

                        case dal::InputCtrlFlag::consumed:
                            goto endWidgetsLoop;
                        case dal::InputCtrlFlag::ignored:
                            goto endWidgetsLoop;
                        case dal::InputCtrlFlag::owned:
                            state.m_owner = bgWidget;
                            goto endWidgetsLoop;
                        default:
                            dalAbort("Shouldn't reach here, the enum's index is: {}"_format(static_cast<int>(ctrlFlag)));

                        }
                    }

                endWidgetsLoop:
                    continue;
                }
            }  // for

            tq.clear();
        }  // func dispatch

        void notifyWidgetRemoved(dal::Widget2* const w) {
            for ( auto& [id, state] : this->m_states ) {
                if ( w == state.m_owner ) {
                    state.m_owner = nullptr;
                    return;
                }
            }
        }

    private:
        TouchState& getOrMakeTouchState(const dal::touchID_t id, states_t& states) const {
            auto found = states.find(id);
            if ( this->m_states.end() != found ) {
                return found->second;
            }
            else {
                return states.emplace(id, TouchState{}).first->second;
            }
        }

    } g_touchMas;  // class TouchStatesMaster

}


// TextStreamChannel
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
        , m_winWidth(static_cast<float>(width))
        , m_winHeight(static_cast<float>(height))
        , m_backgroundWidget(nullptr)
        , m_backgroundOwned(false)
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

        // Widgets 2
        {
            auto widget = new Label2(nullptr, this->m_unicodes);

            widget->setText("fuck you");
            widget->setSize(50.0f, 20.0f);
            widget->setBackgroundColor(0.0f, 0.0f, 0.0f, 0.5f);

            this->m_toDelete2.push_front(widget);
            this->m_widgets2.push_front(widget);
            
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
        for ( auto w : this->m_toDelete ) {
            delete w;
        }
        for ( auto w : this->m_toDelete2 ) {
            delete w;
        }
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
        this->m_winWidth = static_cast<float>(width);
        this->m_winHeight = static_cast<float>(height);

        for ( auto& wid : this->m_widgets ) {
            wid->onResize(width, height);
        }

        this->m_backgroundWidget->onParentResize(this->m_winWidth, this->m_winHeight);
        for ( auto w : this->m_widgets2 ) {
            w->onParentResize(this->m_winWidth, this->m_winHeight);
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

    void OverlayMaster::updateInputs(void) {
        g_touchMas.dispatch(this->m_winWidth, this->m_winHeight, this->m_widgets2, this->m_backgroundWidget);
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

        {
            if ( nullptr != this->m_backgroundWidget ) {
                this->m_backgroundWidget->render(uniloc, this->m_winWidth, this->m_winHeight);
            }

            const auto end = this->m_widgets2.rend();
            for ( auto iter = this->m_widgets2.rbegin(); iter != end; ++iter ) {
                (*iter)->render(uniloc, this->m_winWidth, this->m_winHeight);
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

    void OverlayMaster::giveWidgetOwnership(Widget2* const w) {
        this->m_widgets2.push_front(w);
        this->m_toDelete2.push_back(w);
    }

    void OverlayMaster::giveWidgetRef(Widget* const w) {
        for ( auto widget : this->m_widgets ) {
            widget->onFocusChange(false);
        }

        this->m_widgets.push_front(w);

        w->onFocusChange(true);
    }

    void OverlayMaster::giveWidgetRef(Widget2* const w) {
        this->m_widgets2.push_front(w);
    }

    bool OverlayMaster::removeWidgetRef(Widget2* const w) {
        const auto end = this->m_widgets2.end();
        const auto found = std::find(this->m_widgets2.begin(), end, w);
        if ( end != found ) {
            this->m_widgets2.erase(found);
            return true;
        }
        else {
            return false;
        }
    }

    void OverlayMaster::giveBackgroudWidgetRef(Widget2* const w) {
        this->clearBackgroudWidget();
        this->m_backgroundWidget = w;
        this->m_backgroundOwned = false;
    }

    void OverlayMaster::giveBackgroudWidgetOwnership(Widget2* const w) {
        this->clearBackgroudWidget();
        this->m_backgroundWidget = w;
        this->m_backgroundOwned = true;
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

    void OverlayMaster::clearBackgroudWidget(void) {
        if ( nullptr != this->m_backgroundWidget ) {
            if ( this->m_backgroundOwned ) {
                delete this->m_backgroundWidget;
            }
            this->m_backgroundWidget = nullptr;
        }
    }

}