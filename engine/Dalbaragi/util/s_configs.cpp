#include "s_configs.h"

#include <fmt/format.h>

#include <d_logger.h>


using namespace fmt::literals;


// ExternalFuncGod
namespace dal {

    std::pair<unsigned, unsigned> ExternalFuncGod::queryWinSize(void) const {
        if ( !this->m_queryWinSize ) {
            dalWarn("External func not set : queryWinSize");
        }
        else {
            return this->m_queryWinSize();
        }
    }

    void ExternalFuncGod::giveValue_assetMgr(AAssetManager* const ptr) {

#ifdef __ANDROID__
        this->m_assetMgr = ptr;
#endif

    }

    void ExternalFuncGod::giveValue_androidStoragePath(const std::string& path) {

#ifdef __ANDROID__
        this->m_androidStoragePath = path;
#endif

    }

#ifdef __ANDROID__
    AAssetManager* ExternalFuncGod::getAssetMgr(void) const {
        if ( nullptr == this->m_assetMgr ) {
            dalAbort("AAssetManager ptr hasn't been set.");
        }
        else {
            return this->m_assetMgr;
        }
    }

    const std::string& ExternalFuncGod::getAndroidStoragePath(void) const {
        if ( this->m_androidStoragePath.empty() ) {
            dalAbort("Android storage path hasn't been set.");
        }
        else {
            return this->m_androidStoragePath;
        }
    }
#endif

}


// GlobalStateGod
namespace dal {

    GlobalStateGod::GlobalStateGod(void)
        : m_winWidth(0), m_winHeight(0)
    {

    }

    GlobalStateGod::~GlobalStateGod(void) {

    }

}
