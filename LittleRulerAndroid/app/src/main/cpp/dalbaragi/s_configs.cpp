#include "s_configs.h"

#include <fmt/format.h>

#include "s_logger_god.h"


using namespace fmt::literals;


namespace dal {

    ConfigsGod::ConfigsGod(void)
        : mWinWidth(0), mWinHeight(0),
        m_gameState(GlobalGameState::game)
    {
        mHandlerName = "dal::ConfigsGod";
        EventGod::getinst().registerHandler(this, EventType::global_fsm_change);
    }

    ConfigsGod::~ConfigsGod(void) {
        EventGod::getinst().deregisterHandler(this, EventType::global_fsm_change);
    }

    ConfigsGod& ConfigsGod::getinst(void) {
        static ConfigsGod inst;
        return inst;
    }

    void ConfigsGod::onEvent(const EventStatic& e) {
        if ( EventType::global_fsm_change == e.type ) {
            this->m_gameState = static_cast<GlobalGameState>(e.intArg1);
        }
        else {
            const auto eventTypeIndex = static_cast<unsigned int>(e.type);
            dalWarn("dal::ConfigsGod can't handle this event: {}"_format(eventTypeIndex));
        }
    }

    unsigned int ConfigsGod::getWinWidth(void) const {
        return mWinWidth;
    }

    unsigned int ConfigsGod::getWinHeight(void) const {
        return mWinHeight;
    }

    void ConfigsGod::setWinSize(const unsigned int width, const unsigned int height) {
        this->mWinWidth = width;
        this->mWinHeight = height;
    }

    GlobalGameState ConfigsGod::getGlobalGameState(void) const {
        return this->m_gameState;
    }

}