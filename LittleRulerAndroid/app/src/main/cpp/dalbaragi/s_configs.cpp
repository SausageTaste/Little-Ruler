#include "s_configs.h"

#include "s_logger_god.h"


using namespace std::string_literals;


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
        switch ( e.type ) {

        case EventType::global_fsm_change:
            this->m_gameState = static_cast<GlobalGameState>(e.intArg1);
            break;
        default:
            const auto eventTypeIndex = static_cast<int>(e.type);
            LoggerGod::getinst().putWarn("dal::ConfigsGod can't handle this event:"s + std::to_string(eventTypeIndex), __LINE__, __func__, __FILE__);
            break;

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