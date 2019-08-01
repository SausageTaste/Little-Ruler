#include "o_overlay_master.h"

#include <string>
#include <unordered_map>

#include <fmt/format.h>

#include "s_logger_god.h"
#include "s_configs.h"


using namespace std::string_literals;
using namespace fmt::literals;


namespace dal {

    OverlayMaster::OverlayMaster(const ShaderMaster& shaderMas, const unsigned int width, const unsigned int height)
        : m_shaderMas(shaderMas)
        , mGlobalFSM(GlobalGameState::game)
        , m_winWidth(static_cast<float>(width))
        , m_winHeight(static_cast<float>(height))
    {
        ConfigsGod::getinst().setWinSize(width, height);

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
            this->mGlobalFSM = static_cast<GlobalGameState>(e.intArg1);
        }
        else {
            dalWarn("Unhanlded event in OverlayMaster.");
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