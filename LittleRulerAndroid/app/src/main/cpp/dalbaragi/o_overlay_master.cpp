#include "o_overlay_master.h"

#include <string>
#include <unordered_map>

#include <fmt/format.h>

#include "s_logger_god.h"
#include "s_configs.h"
#include "o_widget_textbox.h"


using namespace std::string_literals;
using namespace fmt::literals;


namespace {

    class KeyStatesMaster {

    public:
        void dispatch(const float winWidth, const float winHeight, std::list<dal::Widget2*>& widgets, dal::Widget2* const bgWidget) {
            auto& kq = dal::KeyboardEvtQueueGod::getinst();
            const auto kqSize = kq.getSize();

            for ( unsigned int i = 0; i < kqSize; ++i ) {
                const auto& kevent = kq.at(i);

                for ( auto w : widgets ) {
                    const auto ctrlFlag = w->onKeyInput(kevent, kq.getKeyStates());
                    if ( dal::InputCtrlFlag::ignored != ctrlFlag ) {
                        goto finishWidgetLoop;
                    }
                }

                bgWidget->onKeyInput(kevent, kq.getKeyStates());

            finishWidgetLoop:
                continue;
            }

            kq.clear();
        }

    } g_keyMas;  // KeyStatesMaster


    class TouchStatesMaster {

    private:
        struct TouchState {
            dal::Widget2* m_owner = nullptr;
        };

    private:
        using states_t = std::unordered_map<dal::touchID_t, TouchState>;
        states_t m_states;

    public:
        void dispatch(const float winWidth, const float winHeight, std::list<dal::Widget2*>& widgets, dal::Widget2* const bgWidget) {
            auto& tq = dal::TouchEvtQueueGod::getinst();

            for ( unsigned int i = 0; i < tq.getSize(); i++ ) {
                const auto& tevent = tq.at(i);
                auto& state = this->getOrMakeTouchState(tevent.m_id, this->m_states);

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
                        if ( !w->isPointInside(tevent.m_pos) ) {
                            continue;
                        }

                        const auto ctrlFlag = w->onTouch(tevent);
                        switch ( ctrlFlag ) {

                        case dal::InputCtrlFlag::consumed:
                            this->moveOnTop(w, widgets);
                            goto endWidgetsLoop;
                        case dal::InputCtrlFlag::ignored:
                            continue;
                        case dal::InputCtrlFlag::owned:
                            state.m_owner = w;
                            this->moveOnTop(w, widgets);
                            goto endWidgetsLoop;
                        default:
                            dalAbort("Shouldn't reach here, the enum's index is: {}"_format(static_cast<int>(ctrlFlag)));

                        }
                    }

                    if ( nullptr != bgWidget && bgWidget->isPointInside(tevent.m_pos) ) {
                        const auto ctrlFlag = bgWidget->onTouch(tevent);
                        switch ( ctrlFlag ) {

                        case dal::InputCtrlFlag::consumed:
                            widgets.front()->onFocusChange(false);
                            goto endWidgetsLoop;
                        case dal::InputCtrlFlag::ignored:
                            goto endWidgetsLoop;
                        case dal::InputCtrlFlag::owned:
                            widgets.front()->onFocusChange(false);
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

        static void moveOnTop(dal::Widget2* const w, std::list<dal::Widget2*>& widgets) {
            if ( widgets.front() == w ) {
                widgets.front()->onFocusChange(true);
                return;
            }

            widgets.front()->onFocusChange(false);
            widgets.remove(w);
            widgets.push_front(w);
            widgets.front()->onFocusChange(true);
        }

    } g_touchMas;  // class TouchStatesMaster

}


// TextStreamChannel
namespace dal {

    OverlayMaster::TextStreamChannel::TextStreamChannel(dal::StringBufferBasic& texStream)
        : m_texStream(texStream)
    {

    }

    void OverlayMaster::TextStreamChannel::verbose(const char* const str, const int line, const char* const func, const char* const file) {
        const auto text = "[VERBO] "s + str + '\n';
        this->m_texStream.append(text.data(), text.size());
    }

    void OverlayMaster::TextStreamChannel::debug(const char* const str, const int line, const char* const func, const char* const file) {
        const auto text = "[DEBUG] "s + str + '\n';
        this->m_texStream.append(text.data(), text.size());
    }

    void OverlayMaster::TextStreamChannel::info(const char* const str, const int line, const char* const func, const char* const file) {
        const auto text = "[INFO] "s + str + '\n';
        this->m_texStream.append(text.data(), text.size());
    }

    void OverlayMaster::TextStreamChannel::warn(const char* const str, const int line, const char* const func, const char* const file) {
        const auto text = "[WARN] "s + str + '\n';
        this->m_texStream.append(text.data(), text.size());
    }

    void OverlayMaster::TextStreamChannel::error(const char* const str, const int line, const char* const func, const char* const file) {
        const auto text = "[ERROR] "s + str + '\n';
        this->m_texStream.append(text.data(), text.size());
    }

    void OverlayMaster::TextStreamChannel::fatal(const char* const str, const int line, const char* const func, const char* const file) {
        const auto text = "[FATAL] "s + str + '\n';
        this->m_texStream.append(text.data(), text.size());
    }

}


namespace dal {

    OverlayMaster::OverlayMaster(ResourceMaster& resMas, const ShaderMaster& shaderMas, const unsigned int width, const unsigned int height)
        : m_resMas(resMas)
        , m_shaderMas(shaderMas)
        , mGlobalFSM(GlobalGameState::game)
        , m_winWidth(static_cast<float>(width))
        , m_winHeight(static_cast<float>(height))
    {
        ConfigsGod::getinst().setWinSize(width, height);

        // Widgets 2
        /*{
            {
                auto w = new LineEdit(nullptr);

                w->setPos(20, 50);
                w->setSize(500, 20);

                this->giveWidgetOwnership(w);
            }

            {
                auto w = new TextBox(nullptr);

                w->setPos(20, 80);
                w->setSize(500, 400);
                w->replaceBuffer(script::getLuaStdOutBuffer());

                this->giveWidgetOwnership(w);
            }
        }*/

        // Event Master
        {
            this->mHandlerName = "OverlayMaster";
            EventGod::getinst().registerHandler(this, EventType::global_fsm_change);
        }
    }

    OverlayMaster::~OverlayMaster(void) {
        EventGod::getinst().deregisterHandler(this, EventType::global_fsm_change);

        for ( auto w : this->m_toDelete ) {
            delete w;
        }
        this->m_toDelete.clear();
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

        for ( auto w : this->m_widgets ) {
            w->onParentResize(this->m_winWidth, this->m_winHeight);
        }
    }

    void OverlayMaster::updateInputs(void) {
        //g_touchMas.dispatch(this->m_winWidth, this->m_winHeight, this->m_widgets2, this->m_backgroundWidget);
        //g_keyMas.dispatch(this->m_winWidth, this->m_winHeight, this->m_widgets2, this->m_backgroundWidget);

        {
            auto& tq = dal::TouchEvtQueueGod::getinst();

            for ( unsigned int i = 0; i < tq.getSize(); i++ ) {
                const auto& e = tq.at(i);
                const auto [flag, focused] = this->m_dispatcher.dispatch(this->m_widgets.getIterFront(), this->m_widgets.getEndIterFront(), e);
                this->m_widgets.setFocusOn(focused);
            }  // for

            tq.clear();
        }
        
        {
            auto& kq = dal::KeyboardEvtQueueGod::getinst();
            const auto kqSize = kq.getSize();

            for ( unsigned int i = 0; i < kqSize; ++i ) {
                const auto& e = kq.at(i);
                this->m_dispatcher.dispatch(this->m_widgets.getIterFront(), this->m_widgets.getEndIterFront(), e, kq.getKeyStates());
            }

            kq.clear();
        }
    }

    void OverlayMaster::render(void) const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        auto& uniloc = this->m_shaderMas.useOverlay();

        const auto end = this->m_widgets.getEndIterBack();
        for ( auto iter = this->m_widgets.getIterBack(); iter != end; ++iter ) {
            (*iter)->render(uniloc, this->m_winWidth, this->m_winHeight);
        }
    }

    void OverlayMaster::giveWidgetOwnership(Widget2* const w) {
        this->m_widgets.add(w);
        this->m_toDelete.push_back(w);
    }

    void OverlayMaster::giveWidgetRef(Widget2* const w) {
        this->m_widgets.add(w);
    }

    bool OverlayMaster::removeWidgetRef(Widget2* const w) {
        return this->m_widgets.remove(w);
    }

    void OverlayMaster::giveBackgroudWidgetRef(Widget2* const w) {
        this->m_widgets.addBack(w);
    }

}