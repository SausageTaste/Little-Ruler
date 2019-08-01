#include "s_event.h"

#include <fmt/format.h>

#include "s_logger_god.h"


using namespace fmt::literals;


namespace dal {

    const char* getEventTypeStr(const dal::EventType type) {
        static const char* const map[] = {
            "quit_game",
            "global_fsm_change",
            "eoe"
        };

        return map[static_cast<unsigned int>(type)];
    }

    std::string iEventHandler::getHandlerName(void) {
        return this->mHandlerName;
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
        auto& handlerContainer = this->mHandlers[static_cast<unsigned int>(type)];
        handlerContainer.push_back(handler);

        dalVerbose("Registered event \"{}\" for object \"{}\"."_format(getEventTypeStr(type), handler->getHandlerName()))
    }

    void EventGod::deregisterHandler(iEventHandler* handler, const EventType type) {
        auto& handlerContainer = this->mHandlers[static_cast<unsigned int>(type)];

        const auto end = handlerContainer.end();
        auto iter = std::find(handlerContainer.begin(), end, handler);

        if ( iter != end ) {
            handlerContainer.erase(iter);
            dalVerbose("Deregistered event \"{}\" for object \"{}\"."_format(getEventTypeStr(type), handler->getHandlerName()));
        }
        else {
            dalError("Failed to deregister event \"{}\" for \"{}\"."_format(getEventTypeStr(type), handler->getHandlerName()));
        }
    }

    void EventGod::notifyAll(const EventStatic& e) {
        auto& handlerContainer = mHandlers[static_cast<unsigned int>(e.type)];

        for ( auto h : handlerContainer ) {
            h->onEvent(e);
        }
    }

}