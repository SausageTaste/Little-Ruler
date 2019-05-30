#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}


namespace dal {

    class LuaStdOutput {

    public:
        virtual ~LuaStdOutput(void) = default;
        virtual bool append(const char* const str) = 0;

    };


    namespace script {

        void init_renderMas(void* p);
        void set_outputStream(LuaStdOutput* const ptr);

    }


    class Lua {

    private:
        lua_State* L;

        Lua(void);
        ~Lua(void);

    public:
        static Lua& getinst(void);

        void doString(const char* const t);
        void addGlobalFunction(const char* const identifier, lua_CFunction funcPointer);

    };

}