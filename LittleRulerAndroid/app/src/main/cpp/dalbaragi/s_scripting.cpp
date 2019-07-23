#include "s_scripting.h"

#include <iostream>
#include <string>

#include "s_configs.h"
#include "s_logger_god.h"

#include "x_mainloop.h"


using namespace std::string_literals;


// DefaultOutputStream
namespace {

    dal::StringBufferBasic g_defaultOutput;

}


// External Dependencies
namespace {

    class ExternalDependencies {

    private:
        static dal::RenderMaster* s_renderMas;
        static dal::SceneMaster* s_sceneMas;
        static dal::StringBufferBasic* s_output;

    public:
        static void set_output(dal::StringBufferBasic* ptr) {
            s_output = ptr;
        }

        static dal::StringBufferBasic* get_output(void) {
            return s_output;
        }

        static void init(void* const renderMaster, void* const sceneMaster) {
            s_renderMas = reinterpret_cast<dal::RenderMaster*>(renderMaster);
            s_sceneMas = reinterpret_cast<dal::SceneMaster*>(sceneMaster);
        }

        static dal::RenderMaster* getRenderMas(void) {
            dalAssert(nullptr != s_renderMas);
            return s_renderMas;
        }

        static dal::SceneMaster* getSceneMas(void) {
            dalAssert(nullptr != s_sceneMas);
            return s_sceneMas;
        }

    };

    dal::RenderMaster* ExternalDependencies::s_renderMas = nullptr;
    dal::SceneMaster* ExternalDependencies::s_sceneMas = nullptr;
    dal::StringBufferBasic* ExternalDependencies::s_output = &g_defaultOutput;

}


// Misc functions
namespace {

    int moon_print(lua_State* L) {
        auto output = ExternalDependencies::get_output();
        if ( output == nullptr ) {
            return 0;
        }

        int nargs = lua_gettop(L);

        for ( int i = 1; i <= nargs; ++i ) {
            auto aText = lua_tostring(L, i);
            if ( nullptr == aText ) continue;

            output->append(aText, std::strlen(aText));
            output->append(' ');
        }

        output->append('\n');
        return 0;
    }

}


// Render Master
namespace {

    int moon_setRenderScale(lua_State* L) {
        auto renderMas = ExternalDependencies::getRenderMas();

        const auto numParams = lua_gettop(L);
        if ( numParams < 1 ) return 0;

        const auto param = static_cast<float>(lua_tonumber(L, 1));
        if ( param < 0.1 || param > 3.0 ) return 0;

        renderMas->resizeRenderScale(param);

        return 0;
    }

}


// Scene Master
namespace {

    int moon_loadMap(lua_State* L) {
        const auto numParams = lua_gettop(L);
        if ( numParams < 1 ) {
            return 0;
        }

        const auto param1 = lua_tostring(L, 1);
        ExternalDependencies::getSceneMas()->loadMap(param1);
    }

}


namespace dal {
    namespace script {

        void init(void* const renderMaster, void* const sceneMaster) {
            ExternalDependencies::init(renderMaster, sceneMaster);
        }

        void set_outputStream(StringBufferBasic* const ptr) {
            ExternalDependencies::set_output(ptr);
        }

        StringBufferBasic* getLuaStdOutBuffer(void) {
            return ExternalDependencies::get_output();
        }

    }
}


namespace dal {

    Lua::Lua(void) {
        L = luaL_newstate();
        luaL_openlibs(L);

        this->addGlobalFunction("print", moon_print);

        this->addGlobalFunction("set_render_scale", moon_setRenderScale);
        this->addGlobalFunction("load_map", moon_loadMap);
    }

    Lua::~Lua(void) {
        lua_close(L);
    }

    Lua& Lua::getinst(void) {
        static Lua inst;
        return inst;
    }

    void Lua::doString(const char* const t) {
        // It crashes with script "print(shit)" without shit defined.
        auto err = luaL_dostring(L, t);
        if ( err ) {
            auto srrMsg = lua_tostring(L, -1);

            auto output = ExternalDependencies::get_output();
            if ( output != nullptr ) {
                output->append(srrMsg, std::strlen(srrMsg));
                output->append('\n');
            }

            lua_pop(L, 1);
        }
    }

    void Lua::addGlobalFunction(const char* const identifier, lua_CFunction funcPointer) {
        const struct luaL_Reg funcArr[] = {
            {identifier, funcPointer},
            {nullptr, nullptr} /* end of array */
        };

        lua_getglobal(L, "_G");
        luaL_setfuncs(L, funcArr, 0);
        lua_pop(L, 1);
    }

}