#ifdef _WIN32

#include <SDL.h>
#include <gl\glew.h>
#include <SDL_opengl.h>

#include <string>
#include <memory>
#include <iostream>
#include <unordered_map>

#include "x_mainloop.h"
#include "s_input_queue.h"


using namespace std::string_literals;


namespace {

	auto& g_logger = dal::LoggerGod::getinst();

	dal::KeySpec mapKeySpec(uint32_t sdlKey) {
		if (SDLK_a <= sdlKey && sdlKey <= SDLK_z) {
			auto index = sdlKey - SDLK_a + int(dal::KeySpec::a);
			return dal::KeySpec(index);
		}
		else if (SDLK_0 <= sdlKey && sdlKey <= SDLK_9) {
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
			if (res == map.end()) {
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

		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_KEYDOWN) {
				dal::KeyboardEvtQueueGod::getinst().emplaceBack(mapKeySpec(e.key.keysym.sym), dal::KeyboardType::down);
			}
			else if (e.type == SDL_KEYUP) {
				dal::KeyboardEvtQueueGod::getinst().emplaceBack(mapKeySpec(e.key.keysym.sym), dal::KeyboardType::up);
			}
			else if (e.type == SDL_MOUSEMOTION) {
				dal::TouchEvtQueueGod::getinst().emplaceBack(float(e.motion.x), float(e.motion.y), dal::TouchType::move, 0);
			}
			else if (e.type == SDL_MOUSEBUTTONDOWN) {
				dal::TouchEvtQueueGod::getinst().emplaceBack(float(e.motion.x), float(e.motion.y), dal::TouchType::down, 0);
			}
			else if (e.type == SDL_MOUSEBUTTONUP) {
				dal::TouchEvtQueueGod::getinst().emplaceBack(float(e.motion.x), float(e.motion.y), dal::TouchType::up, 0);
			}
			else if (e.type == SDL_WINDOWEVENT) {
				if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
					engine->onResize(e.window.data1, e.window.data2);
				}
			}
			else if (e.type == SDL_QUIT) {
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
				if (SDL_Init(SDL_INIT_VIDEO) < 0) {
					dalAbort("Failed to initiate SDL.");
				}

				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
				SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

				if (fullscreen) {
					mWindow = SDL_CreateWindow(
						title, 50, 50, winWidth, winHeight,
						SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN
					);
				}
				else {
					mWindow = SDL_CreateWindow(
						title, 50, 50, winWidth, winHeight,
						SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
					);

				}

				if (nullptr == mWindow) {
					dalAbort("Creating window failed, SDL Error: "s + SDL_GetError());
				}
			}

			// Create context
			mContext = SDL_GL_CreateContext(mWindow);
			if (nullptr == mContext) {
				dalAbort("Creating OpenGL context failed, SDL Error: "s + SDL_GetError());
			}

			if (SDL_GL_SetSwapInterval(0) < 0) {
				dalError("Unable to disable VSync, SDL Error: "s + SDL_GetError())
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
			if (GLEW_OK != glewError) {
				dalAbort("Initializing GLEW failed, glew error: "s + reinterpret_cast<const char*>(glewGetErrorString(glewError)));
			}
		}

	};

}


namespace dal {

	int main_windows(void) try {
		WindowSDL window{ "Little Ruler", 800, 450, false };

		Mainloop::giveScreenResFirst(800, 450);

		std::unique_ptr<Mainloop> engine{ new Mainloop(nullptr) };

		while (true) {
			auto order = pullEventSDL(engine.get());
			switch (order) {

			case InputOrder::quit:
				return 0;
			default:
				break;

			}

			engine->update();
			window.swap();
		}
	}
	catch (const std::exception& e) {
		g_logger.putFatal("An exception thrown: "s + e.what(), __LINE__, __func__, __FILE__); throw;
	}
	catch (const std::string & e) {
		g_logger.putFatal("A string thrown: "s + e, __LINE__, __func__, __FILE__); throw;
	}
	catch (const char* const e) {
		g_logger.putFatal("A char* thrown: "s + e, __LINE__, __func__, __FILE__); throw;
	}
	catch (const int e) {
		g_logger.putFatal("An int thrown: "s + std::to_string(e), __LINE__, __func__, __FILE__); throw;
	}
	catch (...) {
		g_logger.putFatal("Something unkown thrown", __LINE__, __func__, __FILE__); throw;
	}

}

#endif