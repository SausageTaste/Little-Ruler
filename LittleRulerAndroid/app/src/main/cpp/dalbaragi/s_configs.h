#pragma once

#include "s_event.h"
#include "p_globalfsm.h"


namespace dal {

    class GlobalStateGod : public iEventHandler {

    private:
        unsigned int m_winWidth, m_winHeight;
        GlobalGameState m_gameState;

        GlobalStateGod(void);
        ~GlobalStateGod(void);

    public:
        GlobalStateGod(const GlobalStateGod&) = delete;
        GlobalStateGod(GlobalStateGod&&) = delete;
        GlobalStateGod& operator=(const GlobalStateGod&) = delete;
        GlobalStateGod& operator=(GlobalStateGod&&) = delete;

    public:
        static GlobalStateGod& getinst(void) {
            static GlobalStateGod inst;
            return inst;
        }

        virtual void onEvent(const EventStatic& e) override;

        unsigned int getWinWidth(void) const noexcept {
            return this->m_winWidth;
        }
        unsigned int getWinHeight(void) const noexcept {
            return this->m_winHeight;
        }
        void setWinSize(const unsigned int width, const unsigned int height) noexcept {
            this->m_winWidth = width;
            this->m_winHeight = height;
        }

        GlobalGameState getGlobalGameState(void) const noexcept {
            return this->m_gameState;
        }

    };

}