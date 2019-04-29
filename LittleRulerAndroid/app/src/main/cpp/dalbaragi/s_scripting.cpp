#include "s_scripting.h"

#include <iostream>

#include "s_configs.h"
#include "s_logger_god.h"


namespace {

	int moon_print(lua_State* L) {
		int nargs = lua_gettop(L);
		std::cout << "[ LUA ] ";
		for (int i = 1; i <= nargs; ++i) {
			std::cout << lua_tostring(L, i);
		}
		std::cout << std::endl;

		return 0;
	}

}


namespace dal {

	Lua::Lua(void) {
		L = luaL_newstate();
		luaL_openlibs(L);

		this->addGlobalFunction("print", moon_print);
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
		if (err) {
			auto srrMsg = lua_tostring(L, -1);
			LoggerGod::getinst().putInfo(srrMsg);
			lua_pop(L, 1);
		}
	}

	void Lua::addGlobalFunction(const char* const identifier, lua_CFunction funcPointer) {
		const struct luaL_Reg funcArr[] = {
			{identifier, funcPointer},
			{NULL, NULL} /* end of array */
		};

		lua_getglobal(L, "_G");
		luaL_setfuncs(L, funcArr, 0);
		lua_pop(L, 1);
	}

}