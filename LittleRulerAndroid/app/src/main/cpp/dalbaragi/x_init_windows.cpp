#ifdef _WIN32

#include <SDL.h>
#include <gl\glew.h>
#include <SDL_opengl.h>
#include <fmt/format.h>

#include "x_mainloop.h"
#include "s_input_queue.h"
#include "s_logger_god.h"


using namespace fmt::literals;


namespace {

    constexpr unsigned int initWidth = 1280;
    constexpr unsigned int initHeight = 720;

    dal::KeySpec mapKeySpec(uint32_t sdlKey) {
        if ( SDLK_a <= sdlKey && sdlKey <= SDLK_z ) {
            auto index = sdlKey - SDLK_a + int(dal::KeySpec::a);
            return dal::KeySpec(index);
        }
        else if ( SDLK_0 <= sdlKey && sdlKey <= SDLK_9 ) {
            auto index = sdlKey - SDLK_0 + int(dal::KeySpec::n0);
            return dal::KeySpec(index);
        }
        else {
            static const std::unordered_map<uint32_t, dal::KeySpec> map{
                {SDLK_BACKQUOTE, dal::KeySpec::backquote},
                {SDLK_MINUS, dal::KeySpec::minus},
                {SDLK_EQUALS, dal::KeySpec::equal},
                {SDLK_LEFTBRACKET, dal::KeySpec::lbracket},
                {SDLK_RIGHTBRACKET, dal::KeySpec::rbracket},
                {SDLK_BACKSLASH, dal::KeySpec::backslash},
                {SDLK_SEMICOLON, dal::KeySpec::semicolon},
                {SDLK_QUOTE, dal::KeySpec::quote},
                {SDLK_COMMA, dal::KeySpec::comma},
                {SDLK_PERIOD, dal::KeySpec::period},
                {SDLK_SLASH, dal::KeySpec::slash},

                {SDLK_SPACE, dal::KeySpec::space},
                {SDLK_RETURN, dal::KeySpec::enter},
                {SDLK_BACKSPACE, dal::KeySpec::backspace},
                {SDLK_TAB, dal::KeySpec::tab},

                {SDLK_ESCAPE, dal::KeySpec::escape},
                {SDLK_LSHIFT, dal::KeySpec::lshfit},
                {SDLK_RSHIFT, dal::KeySpec::rshfit},
                {SDLK_LCTRL, dal::KeySpec::lctrl},
                {SDLK_RCTRL, dal::KeySpec::rctrl},
                {SDLK_LALT, dal::KeySpec::lalt},
                {SDLK_RALT, dal::KeySpec::ralt},
                {SDLK_UP, dal::KeySpec::up},
                {SDLK_DOWN, dal::KeySpec::down},
                {SDLK_LEFT, dal::KeySpec::left},
                {SDLK_RIGHT, dal::KeySpec::right},
            };

            auto res = map.find(sdlKey);
            if ( res == map.end() ) {
                return dal::KeySpec::unknown;
            }
            else {
                return res->second;
            }
        }
    }

    enum class InputOrder { nothing, quit };

    InputOrder pullEventSDL(dal::Mainloop* const engine) {
        SDL_Event e;

        while ( SDL_PollEvent(&e) != 0 ) {
            if ( e.type == SDL_KEYDOWN ) {
                dal::KeyboardEvtQueueGod::getinst().emplaceBack(mapKeySpec(e.key.keysym.sym), dal::KeyActionType::down);
            }
            else if ( e.type == SDL_KEYUP ) {
                dal::KeyboardEvtQueueGod::getinst().emplaceBack(mapKeySpec(e.key.keysym.sym), dal::KeyActionType::up);
            }
            else if ( e.type == SDL_MOUSEMOTION ) {
                dal::TouchEvtQueueGod::getinst().emplaceBack(float(e.motion.x), float(e.motion.y), dal::TouchActionType::move, 0);
            }
            else if ( e.type == SDL_MOUSEBUTTONDOWN ) {
                dal::TouchEvtQueueGod::getinst().emplaceBack(float(e.motion.x), float(e.motion.y), dal::TouchActionType::down, 0);
            }
            else if ( e.type == SDL_MOUSEBUTTONUP ) {
                dal::TouchEvtQueueGod::getinst().emplaceBack(float(e.motion.x), float(e.motion.y), dal::TouchActionType::up, 0);
            }
            else if ( e.type == SDL_WINDOWEVENT ) {
                if ( e.window.event == SDL_WINDOWEVENT_RESIZED ) {
                    engine->onResize(e.window.data1, e.window.data2);
                }
            }
            else if ( e.type == SDL_QUIT ) {
                return InputOrder::quit;
            }
        }

        return InputOrder::nothing;
    }

}


namespace {

    class WindowSDL {

    private:
        SDL_Window* mWindow;
        SDL_GLContext mContext;

    public:
        WindowSDL(const char* const title, int winWidth, int winHeight, bool fullscreen) {
            // Init window
            {
                if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
                    dalAbort("Failed to initiate SDL.");
                }

                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
                SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

                if ( fullscreen ) {
                    this->mWindow = SDL_CreateWindow(
                        title, 50, 50, winWidth, winHeight,
                        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN
                    );
                }
                else {
                    this->mWindow = SDL_CreateWindow(
                        title, 50, 50, winWidth, winHeight,
                        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
                    );

                }

                if ( nullptr == this->mWindow ) {
                    dalAbort("Creating window failed, SDL Error: {}"_format(SDL_GetError()));
                }
            }

            // Create context
            this->mContext = SDL_GL_CreateContext(this->mWindow);
            if ( nullptr == this->mContext ) {
                dalAbort("Creating OpenGL context failed, SDL Error: {}"_format(SDL_GetError()));
            }

            if ( SDL_GL_SetSwapInterval(0) < 0 ) {
                dalError("Unable to disable VSync, SDL Error: {}"_format(SDL_GetError()));
            }

            this->initGLew();
        }

        ~WindowSDL(void) {
            SDL_DestroyWindow(mWindow);
            mWindow = nullptr;

            SDL_Quit();
        }

        void swap(void) {
            SDL_GL_SwapWindow(this->mWindow);
        }

    private:
        static void initGLew(void) {
            glewExperimental = GL_TRUE;
            const GLenum glewError = glewInit();
            if ( GLEW_OK != glewError ) {
                const auto glewErrMsg = reinterpret_cast<const char*>(glewGetErrorString(glewError));
                dalAbort("Initializing GLEW failed, glew error: {}"_format(glewErrMsg));
            }
        }

    };

}


namespace dal {

    int main_windows(void) {
        WindowSDL window{ "Little Ruler", initWidth, initHeight, false };

        std::unique_ptr<Mainloop> engine{ new Mainloop(initWidth, initHeight) };

        while ( true ) {
            auto order = pullEventSDL(engine.get());
            switch ( order ) {

            case InputOrder::quit:
                return 0;
            default:
                break;

            }

            engine->update();
            window.swap();
        }
    }

}

#endif