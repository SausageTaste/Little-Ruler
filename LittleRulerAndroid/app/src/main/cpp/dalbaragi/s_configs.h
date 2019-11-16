#pragma once

#include <functional>

#include <glm/glm.hpp>

#include "s_event.h"
#include "p_globalfsm.h"


class AAssetManager;


namespace dal {

    class ExternalFuncGod {

    private:
        std::function<void(bool)> m_setFscreen;
        std::function<std::pair<unsigned, unsigned>(void)> m_queryWinSize;

#ifdef __ANDROID__
        // Android filesystem
        AAssetManager* m_assetMgr = nullptr;
        std::string m_androidStoragePath;
#endif

    public:
        ExternalFuncGod(const ExternalFuncGod&) = delete;
        ExternalFuncGod(ExternalFuncGod&&) = delete;
        ExternalFuncGod& operator=(const ExternalFuncGod&) = delete;
        ExternalFuncGod& operator=(ExternalFuncGod&&) = delete;

    private:
        ExternalFuncGod(void) = default;

    public:
        static ExternalFuncGod& getinst(void) {
            static ExternalFuncGod inst;
            return inst;
        }

        void giveFunc_queryWinSize(std::function<std::pair<unsigned, unsigned>(void)> func_queryWinSize) {
            this->m_queryWinSize = func_queryWinSize;
        }
        std::pair<unsigned, unsigned> queryWinSize(void) const;

        void giveFunc_setFscreen(std::function<void(bool)> func) {
            this->m_setFscreen = func;
        }
        void setFscreen(const bool yes) const {
            this->m_setFscreen(yes);
        }

        void giveValue_assetMgr(AAssetManager* const ptr);
        void giveValue_androidStoragePath(const std::string& path);

#ifdef __ANDROID__
        const std::string& getAndroidStoragePath(void) const;
        AAssetManager* getAssetMgr(void) const;
#endif

    };


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

        virtual void onEvent(const EventStatic& e) override;

        unsigned int getWinWidth(void) const noexcept {
            return this->m_winWidth;
        }
        unsigned int getWinHeight(void) const noexcept {
            return this->m_winHeight;
        }
        glm::vec2 getWinSizeFloat(void) const {
            return glm::vec2{
                this->m_winWidth,
                this->m_winHeight
            };
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
