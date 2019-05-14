#include "s_event.h"

#include <string>

#include "s_logger_god.h"


using namespace std::string_literals;


namespace dal {

	const char* getEventTypeStr(const dal::EventType type) {
		static const char* const map[] = {
			"quit_game",
			"window_resize",
			"touch_event",
			"touch_tap",
			"global_fsm_change"
		};

		return map[int(type)];
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
		LoggerGod::getinst().putTrace(
			"Registered for EventType::"s + getEventTypeStr(type) + ": "s + handler->getHandlerName()
		);
	}

	void EventGod::deregisterHandler(iEventHandler* handler, const EventType type) {
		auto& handlerContainer = mHandlers[int(type)];

		for (auto it = handlerContainer.begin(); it != handlerContainer.end(); ++it) {
			auto ihandler = *it;
			if (ihandler == handler) {
				it = handlerContainer.erase(it);
				LoggerGod::getinst().putTrace(
					"Deregistered for EventType::"s + getEventTypeStr(type) + ": "s + handler->getHandlerName()
				);
				return;
			}
		}

		// If given handler was not found.
		LoggerGod::getinst().putWarn(
			"Failed to deregister from EventType::"s + getEventTypeStr(type) + " for "s + handler->getHandlerName()
		);
	}

	void EventGod::notifyAll(const EventStatic& e) {
		auto& handlerContainer = mHandlers[int(e.type)];
		
		for (auto h : handlerContainer) {
			h->onEvent(e);
		}
	}

}