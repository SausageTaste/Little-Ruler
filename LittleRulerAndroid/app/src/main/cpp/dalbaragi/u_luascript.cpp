#include "u_luascript.h"

#include <fmt/format.h>

#include "s_logger_god.h"
#include "s_configs.h"
#include "u_fileclass.h"

#include "x_mainloop.h"


using namespace fmt::literals;


// Dependencies
namespace {

    dal::Mainloop* g_mainloop = nullptr;
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

        void reserve(const size_t s) {
            this->m_arr.reserve(s);
        }

    };


    dal::LuaState* findLuaState(lua_State* const L) {
        lua_getglobal(L, "_G");              // Stack 1
        lua_pushstring(L, "_dal_luastate");  // Stack 2
        lua_gettable(L, -2);                 // Stack 2
        auto ptr = lua_touserdata(L, -1);    // Stack 2
        dalAssert(nullptr != ptr);
        auto luaState = *reinterpret_cast<dal::LuaState**>(ptr);
        lua_pop(L, 2);                       // Stack 0

        return luaState;
    }

    void addPtrToGlobal(lua_State* const L, void* const p) {
        lua_getglobal(L, "_G");                          // 1
        lua_pushstring(L, "_dal_luastate");              // 2
        auto added = lua_newuserdata(L, sizeof(void*));  // 3
        auto userdata = reinterpret_cast<void**>(added);
        *userdata = p;
        lua_settable(L, -3);                             // 1
        lua_pop(L, 1);                                   // 0
    }

    void addFunc(lua_State* const L, const char* const name, const lua_CFunction func) {
        luaL_Reg funcs[] = {
            { name   , func    },
            { nullptr, nullptr }
        };

        lua_getglobal(L, "_G");      // 1
        luaL_setfuncs(L, funcs, 0);  // 1
        lua_pop(L, 1);               // 0
    }

}



// Libs
namespace {

    class ILuaLib {

    private:
        std::vector<std::pair<std::string, lua_CFunction>> m_funcs;

    public:
        void addFunction(const char* const name, lua_CFunction func) {
            this->m_funcs.push_back({ name, func });
        }

    };

}


// Misc functions
namespace {

    int moon_print(lua_State* const L) {
        std::string buffer;
        auto luaState = findLuaState(L);

        const auto nargs = lua_gettop(L);
        for ( int i = 1; i <= nargs; ++i ) {
            if ( lua_isnil(L, i) ) {
                break;
            }

            auto arg = lua_tostring(L, i);
            if ( nullptr == arg ) {
                continue;
            }

            buffer.append(arg);
            buffer += ' ';
        }

        if ( !buffer.empty() ) {
            luaState->appendTextLine(buffer.data(), buffer.size() - 1);
        }

        return 0;
    }


    int set_fullscreen(lua_State* const L) {
        const auto nargs = lua_gettop(L);
        if ( nargs < 1 ) {
            return -1;
        }

        auto& external = dal::ExternalFuncGod::getinst();

        auto flagFscreen = static_cast<bool>(lua_toboolean(L, 1));
        external.setFscreen(flagFscreen);
        auto winSize = external.queryWinSize();
        g_mainloop->onResize(winSize.first, winSize.second);

        return 0;
    }

    int set_render_scale(lua_State* const L) {
        const auto nargs = lua_gettop(L);
        if ( nargs < 1 ) {
            return -1;
        }

        const auto scale = lua_tonumber(L, 1);
        g_renderMas->resizeRenderScale(scale);

        return 0;
    }

    int exec_res(lua_State* const L) {
        const auto nargs = lua_gettop(L);
        if ( nargs < 1 ) {
            return -1;
        }

        const auto arg = lua_tostring(L, 1);
        std::string buffer;
        if ( !dal::futil::getRes_text(arg, buffer) ) {
            return -1;
        }

        auto luaState = findLuaState(L);
        luaState->exec(buffer.c_str());

        return 0;
    }


    //name of this function is not flexible
    int luaopen_mylib(lua_State* L) {
        static const struct luaL_Reg funcs[] = {
            { "set_fullscreen", set_fullscreen },
            { "exec_res", exec_res },
            { "set_render_scale", set_render_scale },
            { nullptr, nullptr }
        };

        luaL_newlib(L, funcs);
        return 1;
    }

}


namespace dal {

    LuaState::LuaState(void)
        : m_lua(luaL_newstate())
        , m_strbuf(nullptr)
    {
        luaL_openlibs(this->m_lua);

        addPtrToGlobal(this->m_lua, this);
        addFunc(this->m_lua, "print", moon_print);

        luaL_requiref(this->m_lua, "mylib", luaopen_mylib, 0);
    }

    LuaState::~LuaState(void) {
        lua_close(this->m_lua);
        this->m_lua = nullptr;
    }

    StringBufferBasic* LuaState::replaceStrbuf(StringBufferBasic* const strbuf) {
        const auto tmp = this->m_strbuf;
        this->m_strbuf = strbuf;
        return tmp;
    }

    bool LuaState::appendTextLine(const char* const buf, const size_t bufSize) {
        if ( nullptr != this->m_strbuf ) {
            this->m_strbuf->append(buf, bufSize);
            return this->m_strbuf->append('\n');
        }
        else {
            return false;
        }
    }

    void LuaState::exec(const char* const statements) {
        if ( luaL_dostring(this->m_lua, statements) ) {
            auto errMsg = fmt::format("[LUA] {}", lua_tostring(this->m_lua, -1));;
            //this->appendTextLine(errMsg.data(), errMsg.size());
            dalError(errMsg);
            lua_pop(this->m_lua, 1);
        }
    }

    // Static

    void LuaState::giveDependencies(void* const mainloop, void* const renderMas) {
        g_mainloop = reinterpret_cast<Mainloop*>(mainloop);
        g_renderMas = reinterpret_cast<RenderMaster*>(renderMas);
    }

}
