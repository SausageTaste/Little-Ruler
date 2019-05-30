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


namespace {

    class DefaultOutputStream : public dal::LuaStdOutput {

        virtual bool append(const char* const str) override {
            std::cout << str;

            return true;
        }

    } g_defaultOutput;

}


// External Dependencies
namespace {

    class ExternalDependencies {

    private:
        static dal::RenderMaster* s_renderMas;
        static dal::LuaStdOutput* s_output;

    public:
        static void init_renderMas(void* p) {
            s_renderMas = reinterpret_cast<dal::RenderMaster*>(p);
        }

        static dal::RenderMaster* get_renderMas(void) {
            if ( nullptr == s_renderMas ) {
                dalAbort("RenderMaster has not passed to Lua.");
            }

            return s_renderMas;
        }

        static void set_output(dal::LuaStdOutput* ptr) {
            s_output = ptr;
        }

        static dal::LuaStdOutput* get_output(void) {
            return s_output;
        }

    };

    dal::RenderMaster* ExternalDependencies::s_renderMas = nullptr;
    dal::LuaStdOutput* ExternalDependencies::s_output = &g_defaultOutput;

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

            output->append(aText);
            output->append(" ");
        }

        output->append("\n");
        return 0;
    }

}


// Render Master
namespace {

    int moon_setRenderScale(lua_State* L) {
        auto renMas = ExternalDependencies::get_renderMas();

        const auto numParams = lua_gettop(L);
        if ( numParams < 1 ) return 0;

        const auto param = static_cast<float>(lua_tonumber(L, 1));
        if ( param < 0.1 || param > 3.0 ) return 0;

        renMas->setRenderScale(param);

        return 0;
    }

    int moon_printResReports(lua_State* L) {
        auto renMas = ExternalDependencies::get_renderMas();
        auto output = ExternalDependencies::get_output();

        std::vector<dal::Package::ResourceReport> reports;
        renMas->m_resMas.getResReports(reports);

        for ( auto& x : reports ) {
            const auto str = x.getStr();
            output->append(str.c_str());
            output->append("\n");
        }

        return 0;
    }

}


namespace dal {
    namespace script {

        void init_renderMas(void* p) {
            ExternalDependencies::init_renderMas(p);
        }

        void set_outputStream(LuaStdOutput* const ptr) {
            ExternalDependencies::set_output(ptr);
        }

    }
}


namespace dal {

    Lua::Lua(void) {
        L = luaL_newstate();
        luaL_openlibs(L);

        this->addGlobalFunction("print", moon_print);

        this->addGlobalFunction("set_render_scale", moon_setRenderScale);
        this->addGlobalFunction("print_res_reports", moon_printResReports);
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
                output->append(srrMsg);
                output->append("\n");
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