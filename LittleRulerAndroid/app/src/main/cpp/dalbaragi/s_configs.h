#pragma once

#include "s_event.h"
#include "p_globalfsm.h"


namespace dal {

    class GlobalStateGod : public iEventHandler {

    private:
        unsigned int mWinWidth, mWinHeight;
        GlobalGameState m_gameState;

        GlobalStateGod(void);
        ~GlobalStateGod(void);

    public:
        GlobalStateGod(const GlobalStateGod&) = delete;
        GlobalStateGod(GlobalStateGod&&) = delete;
        GlobalStateGod& operator=(const GlobalStateGod&) = delete;
        GlobalStateGod& operator=(GlobalStateGod&&) = delete;

    public:
        static GlobalStateGod& getinst(void);

        virtual void onEvent(const EventStatic& e) override;

        unsigned int getWinWidth(void) const;
        unsigned int getWinHeight(void) const;
        void setWinSize(const unsigned int width, const unsigned int height);

        GlobalGameState getGlobalGameState(void) const;

    };

}