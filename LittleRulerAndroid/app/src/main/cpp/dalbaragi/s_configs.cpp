#include "s_configs.h"

#include <fmt/format.h>

#include "s_logger_god.h"


using namespace fmt::literals;


namespace dal {

    GlobalStateGod::GlobalStateGod(void)
        : mWinWidth(0), mWinHeight(0),
        m_gameState(GlobalGameState::game)
    {
        mHandlerName = "dal::GlobalStateGod";
        EventGod::getinst().registerHandler(this, EventType::global_fsm_change);
    }

    GlobalStateGod::~GlobalStateGod(void) {
        EventGod::getinst().deregisterHandler(this, EventType::global_fsm_change);
    }

    GlobalStateGod& GlobalStateGod::getinst(void) {
        static GlobalStateGod inst;
        return inst;
    }

    void GlobalStateGod::onEvent(const EventStatic& e) {
        if ( EventType::global_fsm_change == e.type ) {
            this->m_gameState = static_cast<GlobalGameState>(e.intArg1);
        }
        else {
            const auto eventTypeIndex = static_cast<unsigned int>(e.type);
            dalWarn("dal::GlobalStateGod can't handle this event: {}"_format(eventTypeIndex));
        }
    }

    unsigned int GlobalStateGod::getWinWidth(void) const {
        return mWinWidth;
    }

    unsigned int GlobalStateGod::getWinHeight(void) const {
        return mWinHeight;
    }

    void GlobalStateGod::setWinSize(const unsigned int width, const unsigned int height) {
        this->mWinWidth = width;
        this->mWinHeight = height;
    }

    GlobalGameState GlobalStateGod::getGlobalGameState(void) const {
        return this->m_gameState;
    }

}