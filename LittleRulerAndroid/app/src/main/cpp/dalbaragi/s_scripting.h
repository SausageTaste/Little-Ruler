#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}


namespace dal {

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