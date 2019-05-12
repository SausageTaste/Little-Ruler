#include "s_scripting.h"

#include <iostream>
#include <string>

#include "s_configs.h"
#include "s_logger_god.h"

#include "p_render_master.h"


using namespace std::string_literals;


// Global variables
namespace {

	auto& g_logger = dal::LoggerGod::getinst();

}


// External Dependencies
namespace {

	class ExternalDependencies {

	private:
		static dal::RenderMaster* s_renderMas;

	public:
		static void init_renderMas(void* p) {
			s_renderMas = reinterpret_cast<dal::RenderMaster*>(p);
		}

		static dal::RenderMaster* get_renderMas(void) {
			if (nullptr == s_renderMas) {
				g_logger.putFatal("RenderMaster has not passed to Lua.");
				throw - 1;
			}

			return s_renderMas;
		}

	};

	dal::RenderMaster* ExternalDependencies::s_renderMas = nullptr;

}


// Misc functions
namespace {

	int moon_print(lua_State* L) {
		int nargs = lua_gettop(L);
		std::cout << "[ LUA ] ";
		for (int i = 1; i <= nargs; ++i) {
			auto aText = lua_tostring(L, i);
			if (nullptr == aText) continue;
			std::cout << aText;
		}
		std::cout << std::endl;

		return 0;
	}

}


// Render Master
namespace {

	int moon_setRenderScale(lua_State* L) {
		auto renMas = ExternalDependencies::get_renderMas();
		
		const auto numParams = lua_gettop(L);
		if (numParams < 1) return 0;

		const auto param = static_cast<float>(lua_tonumber(L, 1));
		if (param < 0.1 || param > 3.0) return 0;

		renMas->setRenderScale(param);

		return 0;
	}

	int moon_addActor(lua_State* L) {
		auto renMas = ExternalDependencies::get_renderMas();

		const auto numParams = lua_gettop(L);
		if (numParams < 2) return 0;

		auto modelID = lua_tostring(L, 1);
		auto actorName = lua_tostring(L, 2);
		renMas->m_scene.addActorForModel(modelID, actorName);

		return 0;
	}

	int moon_printResReports(lua_State* L) {
		auto renMas = ExternalDependencies::get_renderMas();
		std::vector<dal::Package::ResourceReport> reports;
		renMas->m_resMas.getResReports(reports);

		for (auto& x : reports) {
			x.print();
		}

		return 0;
	}

}


namespace dal {
	namespace script {

		void init_renderMas(void* p) {
			ExternalDependencies::init_renderMas(p);
		}

	}
}


namespace dal {

	Lua::Lua(void) {
		L = luaL_newstate();
		luaL_openlibs(L);

		this->addGlobalFunction("print", moon_print);

		this->addGlobalFunction("setRenderScale", moon_setRenderScale);
		this->addGlobalFunction("addActor", moon_addActor);
		this->addGlobalFunction("printResReports", moon_printResReports);
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
			{nullptr, nullptr} /* end of array */
		};

		lua_getglobal(L, "_G");
		luaL_setfuncs(L, funcArr, 0);
		lua_pop(L, 1);
	}

}