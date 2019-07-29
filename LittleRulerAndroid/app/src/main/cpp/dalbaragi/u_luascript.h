#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}


namespace dal {

    class LuaState {

    private:
        lua_State* m_lua;

    public:
        LuaState(const LuaState&) = delete;
        LuaState(LuaState&&) = delete;
        LuaState& operator=(const LuaState&) = delete;
        LuaState& operator=(LuaState&&) = delete;

    public:
        LuaState(void);
        ~LuaState(void);

        void exec(const char* const statements);

        // This is gonna be called from Mainloop's ctor and dtor.
        static void giveDependencies(void* const renderMas);

    };

}