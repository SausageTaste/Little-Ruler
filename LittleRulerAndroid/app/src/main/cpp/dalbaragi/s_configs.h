#pragma once

#include "s_event.h"
#include "p_globalfsm.h"


namespace dal {

	class ConfigsGod : public iEventHandler {

	private:
		unsigned int mWinWidth = 0, mWinHeight = 0;
		GlobalGameState m_gameState = GlobalGameState::game;

		ConfigsGod(void);
		~ConfigsGod(void);
		ConfigsGod(const ConfigsGod&) = delete;
		ConfigsGod& operator=(const ConfigsGod&) = delete;

	public:
		static ConfigsGod& getinst(void);

		virtual void onEvent(const EventStatic& e) override;

		unsigned int getWinWidth(void) const;
		unsigned int getWinHeight(void) const;

		GlobalGameState getGlobalGameState(void) const;

	};

}