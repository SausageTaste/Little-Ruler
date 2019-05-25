#pragma once

#include <string>

#if defined(_WIN32)
	#include <gl\glew.h>
	#include <SDL_opengl.h>
#elif defined(__ANDROID__)
	#include <GLES3/gl3.h>
#else
	#error "Unkown platform"
#endif

#define dalGLWarn(void) dal::_logGLError(__LINE__, __func__, __FILE__)


namespace dal {

	void clearGLError(void);

	void _logGLError(const int line, const char* const func, const char* const file);

}