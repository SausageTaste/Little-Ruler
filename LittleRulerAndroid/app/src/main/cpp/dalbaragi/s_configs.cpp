#include "s_configs.h"

#include "s_logger_god.h"


namespace dal {

	ConfigsGod::ConfigsGod(void) {
		mHandlerName = "dal::ConfigsGod";
		EventGod::getinst().registerHandler(this, EventType::window_resize);
	}

	ConfigsGod::~ConfigsGod(void) {
		EventGod::getinst().deregisterHandler(this, EventType::window_resize);
	}

	ConfigsGod& ConfigsGod::getinst(void) {
		static ConfigsGod inst;
		return inst;
	}

	void ConfigsGod::onEvent(const EventStatic& e) {
		switch (e.type) {

		case EventType::window_resize:
			mWinWidth = (unsigned int)e.intArg1;
			mWinHeight = (unsigned int)e.intArg2;
			break;
		default:
			LoggerGod::getinst().putWarn("dal::ConfigsGod can't handle this event:");

		}
	}

	unsigned int ConfigsGod::getWinWidth(void) const {
		return mWinWidth;
	}

	unsigned int ConfigsGod::getWinHeight(void) const {
		return mWinHeight;
	}

}