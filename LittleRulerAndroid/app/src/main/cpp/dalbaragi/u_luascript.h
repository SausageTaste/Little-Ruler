#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include "u_strbuf.h"


namespace dal {

    class LuaState {

    private:
        lua_State* m_lua;
        StringBufferBasic* m_strbuf;

    public:
        LuaState(const LuaState&) = delete;
        LuaState(LuaState&&) = delete;
        LuaState& operator=(const LuaState&) = delete;
        LuaState& operator=(LuaState&&) = delete;

    public:
        LuaState(void);
        ~LuaState(void);

        StringBufferBasic* replaceStrbuf(StringBufferBasic* const strbuf);
        bool appendTextLine(const char* const buf, const size_t bufSize);
        void exec(const char* const statements);

        // This is gonna be called from Mainloop's ctor and dtor.
        static void giveDependencies(void* const mainloop, void* const renderMas);

    };

}
