#pragma once

#include <functional>

#include "s_event.h"
#include "p_globalfsm.h"


namespace dal {

    class GlobalStateGod : public iEventHandler {

    private:
        unsigned int m_winWidth, m_winHeight;
        GlobalGameState m_gameState;
        std::function<void(bool)> m_fullscreenToggleFunc;

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

        void giveFscreenToggleFunc(std::function<void(bool)> fullscreenToggle) {
            this->m_fullscreenToggleFunc = fullscreenToggle;
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

        void setFullscreen(const bool yes) {
            if ( this->m_fullscreenToggleFunc ) {
                this->m_fullscreenToggleFunc(yes);
            }
        }

        GlobalGameState getGlobalGameState(void) const noexcept {
            return this->m_gameState;
        }

    };

}