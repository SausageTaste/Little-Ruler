#include "u_luascript.h"

#include "s_logger_god.h"
#include "p_render_master.h"


using namespace std::string_literals;


// Dependencies
namespace {

    dal::RenderMaster* g_renderMas = nullptr;

}


// Utils
namespace {

    class LuaAddFuncArray {

    private:
        std::vector<luaL_Reg> m_arr;

    public:
        void add(const char* const name, lua_CFunction func) {
            auto& added = this->m_arr.emplace_back();
            added.name = name;
            added.func = func;
        }

        void apply(lua_State* const L) {
            this->add(nullptr, nullptr);

            lua_getglobal(L, "_G");
            luaL_setfuncs(L, this->m_arr.data(), 0);
            lua_pop(L, 1);
        }

    };

}


// Misc functions
namespace {

    int moon_print(lua_State* L) {
        std::string buffer;

        const auto nargs = lua_gettop(L);
        for ( int i = 1; i <= nargs; ++i ) {
            auto arg = lua_tostring(L, i);
            if ( nullptr == arg ) {
                continue;
            }

            buffer += arg + ' ';
        }

        dalVerbose(buffer);

        return 0;
    }

}


namespace dal {

    LuaState::LuaState(void) 
        : m_lua(luaL_newstate())
    {
        luaL_openlibs(this->m_lua);
    }

    LuaState::~LuaState(void) {
        lua_close(this->m_lua);
        this->m_lua = nullptr;
    }

    void LuaState::exec(const char* const statements) {
        auto err = luaL_dostring(this->m_lua, statements);
        if ( err ) {
            auto errMsg = lua_tostring(this->m_lua, -1);
            dalError(errMsg);
            lua_pop(this->m_lua, 1);
        }
    }

    // Static

    void LuaState::giveDependencies(void* const renderMas) {
        g_renderMas = reinterpret_cast<RenderMaster*>(renderMas);
    }

}