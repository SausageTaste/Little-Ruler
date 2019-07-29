#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include "u_strbuf.h"


namespace dal {

    namespace script {

        void init(void* const renderMaster, void* const sceneMaster);
        StringBufferBasic* getLuaStdOutBuffer(void);

    }


    class LuaGod {

    private:
        lua_State* L;

        LuaGod(void);
        ~LuaGod(void);

    public:
        static LuaGod& getinst(void);

        void doString(const char* const t);
        void addGlobalFunction(const char* const identifier, lua_CFunction funcPointer);

    };

}