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

    class KeyStatesMaster {

    public:
        void dispatch(const float winWidth, const float winHeight, std::list<dal::Widget2*>& widgets, dal::Widget2* const bgWidget) {
            auto& kq = dal::KeyboardEvtQueueGod::getinst();
            const auto kqSize = kq.getSize();

            for ( unsigned int i = 0; i < kqSize; ++i ) {
                const auto& kevent = kq.at(i);

                unsigned int order = 0;
                for ( auto w : widgets ) {
                    if ( dal::InputCtrlFlag::ignored != w->onKeyInput(kevent, dal::Widget2::KeyAdditionalStates{ order }) ) {
                        break;
                    }

                    ++order;
                }
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

                    if ( nullptr != bgWidget && bgWidget->isPointInside(tevent.x, tevent.y) ) {
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
        , m_unicodes(resMas)
        , mGlobalFSM(GlobalGameState::game)
        , m_texStreamCh(*script::getLuaStdOutBuffer())
        , m_winWidth(static_cast<float>(width))
        , m_winHeight(static_cast<float>(height))
        , m_backgroundWidget(nullptr)
        , m_backgroundOwned(false)
    {
        ConfigsGod::getinst().setWinSize(width, height);

        // Widgets 2
        {
            {
                //auto t = new TextRenderer(nullptr);

                //t->setPos(100.0, 100.0);
                //t->setSize(205.0, 200.0);
                //t->setText("Fuck me.\nthishASFt;ashdljfkasjdASFflAFajsdl;fkjasl;jWEWASFfd;lasjdfljaslfjkdsaf\nAAAA\tAAAAAAAAAAAAAA");

                //this->giveWidgetOwnership(t);
            }

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

        this->m_widgets2.clear();
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

        this->m_backgroundWidget->onParentResize(this->m_winWidth, this->m_winHeight);
        for ( auto w : this->m_widgets2 ) {
            w->onParentResize(this->m_winWidth, this->m_winHeight);
        }
    }

    void OverlayMaster::updateInputs(void) {
        g_touchMas.dispatch(this->m_winWidth, this->m_winHeight, this->m_widgets2, this->m_backgroundWidget);
        g_keyMas.dispatch(this->m_winWidth, this->m_winHeight, this->m_widgets2, this->m_backgroundWidget);
    }

    void OverlayMaster::render(void) const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        auto& uniloc = this->m_shaderMas.useOverlay();

        if ( nullptr != this->m_backgroundWidget ) {
            this->m_backgroundWidget->render(uniloc, this->m_winWidth, this->m_winHeight);
        }

        const auto end = this->m_widgets2.rend();
        for ( auto iter = this->m_widgets2.rbegin(); iter != end; ++iter ) {
            (*iter)->render(uniloc, this->m_winWidth, this->m_winHeight);
        }
    }

    void OverlayMaster::giveWidgetOwnership(Widget2* const w) {
        this->m_widgets2.push_front(w);
        this->m_toDelete2.push_back(w);
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

    // Private

    void OverlayMaster::clearBackgroudWidget(void) {
        if ( nullptr != this->m_backgroundWidget ) {
            if ( this->m_backgroundOwned ) {
                delete this->m_backgroundWidget;
            }
            this->m_backgroundWidget = nullptr;
        }
    }

}