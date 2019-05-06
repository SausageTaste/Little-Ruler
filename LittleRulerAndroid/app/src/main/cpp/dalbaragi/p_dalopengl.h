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


namespace dal {

	void printAllErrorsGL(void);

}