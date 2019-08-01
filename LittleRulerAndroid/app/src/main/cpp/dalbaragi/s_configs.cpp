#include "s_configs.h"

#include <fmt/format.h>

#include "s_logger_god.h"


using namespace fmt::literals;


namespace dal {

    GlobalStateGod::GlobalStateGod(void)
        : m_winWidth(0), m_winHeight(0)
        , m_gameState(GlobalGameState::game)
    {
        mHandlerName = "dal::GlobalStateGod";
        EventGod::getinst().registerHandler(this, EventType::global_fsm_change);
    }

    GlobalStateGod::~GlobalStateGod(void) {
        EventGod::getinst().deregisterHandler(this, EventType::global_fsm_change);
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

}