#pragma once

#include "s_event.h"


namespace dal {

	class ConfigsGod : public iEventHandler {

	private:
		unsigned int mWinWidth, mWinHeight;

		ConfigsGod(void);
		~ConfigsGod(void);
		ConfigsGod(ConfigsGod&) = delete;
		ConfigsGod& operator=(ConfigsGod&) = delete;

	public:
		static ConfigsGod& getinst(void);

		virtual void onEvent(const EventStatic& e) override;

		unsigned int getWinWidth(void) const;

		unsigned int getWinHeight(void) const;

	};

}