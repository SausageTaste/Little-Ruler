#ifdef _WIN32

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <fmt/format.h>

#include <d_logger.h>

#include "x_mainloop.h"
#include "s_input_queue.h"


using namespace fmt::literals;


namespace {

#if 0
    constexpr unsigned int INIT_WIN_WIDTH = 1920;
    constexpr unsigned int INIT_WIN_HEIGHT = 1080;
    constexpr bool FULLSCREEN = true;
#else
    constexpr unsigned int INIT_WIN_WIDTH = 1280;
    constexpr unsigned int INIT_WIN_HEIGHT = 720;
    constexpr bool FULLSCREEN = false;
#endif

    constexpr unsigned int MIN_WIN_WIDTH = 640;
    constexpr unsigned int MIN_WIN_HEIGHT = 480;


    dal::Mainloop* g_engine = nullptr;


    dal::KeySpec mapKeySpec(const int sdlKey) {
        if ( GLFW_KEY_A <= sdlKey && sdlKey <= GLFW_KEY_Z ) {
            auto index = sdlKey - GLFW_KEY_A + int(dal::KeySpec::a);
            return dal::KeySpec(index);
        }
        else if ( GLFW_KEY_0 <= sdlKey && sdlKey <= GLFW_KEY_9 ) {
            auto index = sdlKey - GLFW_KEY_0 + int(dal::KeySpec::n0);
            return dal::KeySpec(index);
        }
        else {
            static const std::unordered_map<uint32_t, dal::KeySpec> map{
                {GLFW_KEY_GRAVE_ACCENT, dal::KeySpec::backquote},
                {GLFW_KEY_MINUS, dal::KeySpec::minus},
                {GLFW_KEY_EQUAL, dal::KeySpec::equal},
                {GLFW_KEY_LEFT_BRACKET, dal::KeySpec::lbracket},
                {GLFW_KEY_RIGHT_BRACKET, dal::KeySpec::rbracket},
                {GLFW_KEY_BACKSLASH, dal::KeySpec::backslash},
                {GLFW_KEY_SEMICOLON, dal::KeySpec::semicolon},
                {GLFW_KEY_APOSTROPHE, dal::KeySpec::quote},
                {GLFW_KEY_COMMA, dal::KeySpec::comma},
                {GLFW_KEY_PERIOD, dal::KeySpec::period},
                {GLFW_KEY_SLASH, dal::KeySpec::slash},

                {GLFW_KEY_SPACE, dal::KeySpec::space},
                {GLFW_KEY_ENTER, dal::KeySpec::enter},
                {GLFW_KEY_BACKSPACE, dal::KeySpec::backspace},
                {GLFW_KEY_TAB, dal::KeySpec::tab},

                {GLFW_KEY_ESCAPE, dal::KeySpec::escape},
                {GLFW_KEY_LEFT_SHIFT, dal::KeySpec::lshfit},
                {GLFW_KEY_RIGHT_SHIFT, dal::KeySpec::rshfit},
                {GLFW_KEY_LEFT_CONTROL, dal::KeySpec::lctrl},
                {GLFW_KEY_RIGHT_CONTROL, dal::KeySpec::rctrl},
                {GLFW_KEY_LEFT_ALT, dal::KeySpec::lalt},
                {GLFW_KEY_RIGHT_ALT, dal::KeySpec::ralt},
                {GLFW_KEY_UP, dal::KeySpec::up},
                {GLFW_KEY_DOWN, dal::KeySpec::down},
                {GLFW_KEY_LEFT, dal::KeySpec::left},
                {GLFW_KEY_RIGHT, dal::KeySpec::right},
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

    /*
    InputOrder pullEventSDL(dal::Mainloop& engine) {
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
                    engine.onResize(e.window.data1, e.window.data2);
                }
            }
            else if ( e.type == SDL_QUIT ) {
                return InputOrder::quit;
            }
        }

        return InputOrder::nothing;
    }
    */


    void callback_error(int error, const char* description) {
        dalError(fmt::format("Error: {}\n", description));
    }

    void callback_keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods) {
        dal::KeyActionType actionType;
        switch ( action ) {

        case GLFW_PRESS:
            actionType = dal::KeyActionType::down;
            break;
        case GLFW_REPEAT:
            actionType = dal::KeyActionType::down;
            break;
        case GLFW_RELEASE:
            actionType = dal::KeyActionType::up;
            break;
        default:
            dalAbort("wtf");

        }

        dal::KeyboardEvtQueueGod::getinst().emplaceBack(mapKeySpec(key), actionType);
    }

    void callback_cursorPos(GLFWwindow* window, double xpos, double ypos) {
        dal::TouchEvtQueueGod::getinst().emplaceBack(xpos, ypos, dal::TouchActionType::move, 0);
    }

    void callback_mouseButton(GLFWwindow* window, int button, int action, int mods) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        if ( button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS ) {
            dal::TouchEvtQueueGod::getinst().emplaceBack(xpos, ypos, dal::TouchActionType::down, 0);
        }
        else if ( button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE ) {
            dal::TouchEvtQueueGod::getinst().emplaceBack(xpos, ypos, dal::TouchActionType::up, 0);
        }
    }

    void callback_resizeFbuf(GLFWwindow* window, int width, int height) {
        if ( width < MIN_WIN_WIDTH )  width = MIN_WIN_WIDTH;
        if ( height < MIN_WIN_HEIGHT ) height = MIN_WIN_HEIGHT;

        if ( nullptr != g_engine ) {
            g_engine->onResize(width, height);
        }
    }

}


namespace {

    class WindowSDL {

    private:
        GLFWwindow* m_window = nullptr;

    public:
        WindowSDL(const char* const title, int winWidth, int winHeight, bool fullscreen) {
            glfwSetErrorCallback(callback_error);

            if ( GLFW_FALSE == glfwInit() ) {
                dalAbort("failed to initialize GLFW");
            }

            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
            this->m_window = glfwCreateWindow(winWidth, winHeight, title, NULL, NULL);
            if ( nullptr == this->m_window ) {
                dalAbort("failed to create window");
            }

            glfwSetKeyCallback(this->m_window, callback_keyEvent);
            glfwSetCursorPosCallback(this->m_window, callback_cursorPos);
            glfwSetMouseButtonCallback(this->m_window, callback_mouseButton);
            glfwSetFramebufferSizeCallback(this->m_window, callback_resizeFbuf);

            glfwSetWindowSizeLimits(this->m_window, MIN_WIN_WIDTH, MIN_WIN_HEIGHT, GLFW_DONT_CARE, GLFW_DONT_CARE);

            glfwMakeContextCurrent(this->m_window);
            if ( 0 == gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) ) {
                dalAbort("failed to load OpenGL functions");
            }
            glfwSwapInterval(0);
        }

        ~WindowSDL(void) {
            glfwDestroyWindow(this->m_window);
            this->m_window = nullptr;

            glfwTerminate();
        }

        void swap(void) {
            glfwSwapBuffers(this->m_window);
        }

        bool needToClose(void) {
            return glfwWindowShouldClose(this->m_window);
        }

        void setFullscreen(const bool yes) {
            dalAbort("not implemented");
        }

        std::pair<unsigned, unsigned> getWinSize(void) {
            int w, h;
            glfwGetFramebufferSize(this->m_window, &w, &h);
            return std::pair<size_t, size_t>(w, h);
        }

        std::pair<double, double> getMousePos(void) {
            double xpos, ypos;
            glfwGetCursorPos(this->m_window, &xpos, &ypos);
            return { xpos, ypos };
        }

    };

}


namespace dal {

    int main_windows(void) {
        WindowSDL window{ "Little Ruler", INIT_WIN_WIDTH, INIT_WIN_HEIGHT, FULLSCREEN };

        {
            auto& extgod = ExternalFuncGod::getinst();

            extgod.giveFunc_setFscreen(
                [&window](const bool fscreen) {
                    window.setFullscreen(fscreen);
                }
            );

            extgod.giveFunc_queryWinSize(
                [&window](void) -> std::pair<unsigned, unsigned> {
                    return window.getWinSize();
                }
            );
        }

        std::unique_ptr<Mainloop> engine{ new Mainloop{ INIT_WIN_WIDTH, INIT_WIN_HEIGHT } };
        g_engine = engine.get();

        while ( !window.needToClose() ) {
            glfwPollEvents();
            engine->update();
            window.swap();
        }

        g_engine = nullptr;
        return 0;
    }

}

#endif
