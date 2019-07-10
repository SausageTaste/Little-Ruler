#include "s_event.h"

#include <string>

#include "s_logger_god.h"


using namespace std::string_literals;


namespace dal {

    const char* getEventTypeStr(const dal::EventType type) {
        static const char* const map[] = {
            "quit_game",
            "global_fsm_change",
            "eoe"
        };

        return map[static_cast<int>(type)];
    }

    std::string iEventHandler::getHandlerName(void) {
        return mHandlerName;
    }

}


namespace dal {

    EventGod::EventGod(void) {

    }

    EventGod& EventGod::getinst(void) {
        static EventGod inst;
        return inst;
    }

    void EventGod::registerHandler(iEventHandler* handler, const EventType type) {
        auto& handlerContainer = mHandlers[int(type)];

        handlerContainer.push_back(handler);
        LoggerGod::getinst().putVerbose(
            "Registered for EventType::"s + getEventTypeStr(type) + ": "s + handler->getHandlerName(), __LINE__, __func__, __FILE__
        );
    }

    void EventGod::deregisterHandler(iEventHandler* handler, const EventType type) {
        auto& handlerContainer = mHandlers[int(type)];

        for ( auto it = handlerContainer.begin(); it != handlerContainer.end(); ++it ) {
            auto ihandler = *it;
            if ( ihandler == handler ) {
                it = handlerContainer.erase(it);
                LoggerGod::getinst().putVerbose(
                    "Deregistered for EventType::"s + getEventTypeStr(type) + ": "s + handler->getHandlerName(), __LINE__, __func__, __FILE__
                );
                return;
            }
        }

        // If given handler was not found.
        LoggerGod::getinst().putWarn(
            "Failed to deregister from EventType::"s + getEventTypeStr(type) + " for "s + handler->getHandlerName(), __LINE__, __func__, __FILE__
        );
    }

    void EventGod::notifyAll(const EventStatic& e) {
        auto& handlerContainer = mHandlers[int(e.type)];

        for ( auto h : handlerContainer ) {
            h->onEvent(e);
        }
    }

}