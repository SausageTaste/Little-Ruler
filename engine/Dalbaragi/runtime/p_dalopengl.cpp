#include "p_dalopengl.h"

#include <string>

#include <spdlog/fmt/fmt.h>

#include <d_logger.h>


using namespace fmt::literals;


namespace {

    char const* gl_error_string(GLenum const err)
    {
        switch ( err ) {
            // opengl 2 errors (8)
        case GL_NO_ERROR:
            return "GL_NO_ERROR";

        case GL_INVALID_ENUM:
            return "GL_INVALID_ENUM";

        case GL_INVALID_VALUE:
            return "GL_INVALID_VALUE";

        case GL_INVALID_OPERATION:
            return "GL_INVALID_OPERATION";
#ifdef _WIN32
        case GL_STACK_OVERFLOW:
            return "GL_STACK_OVERFLOW";

        case GL_STACK_UNDERFLOW:
            return "GL_STACK_UNDERFLOW";

        case GL_OUT_OF_MEMORY:
            return "GL_OUT_OF_MEMORY";

            // opengl 3 errors (1)
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return "GL_INVALID_FRAMEBUFFER_OPERATION";
#endif
            // gles 2, 3 and gl 4 error are handled by the switch above
        default:
            return "unknown error";
        }
    }

}


namespace dal {

    void clearGLError(void) {
        for ( int i = 0; i < 1000; i++ ) {
            const auto err = glGetError();
            if ( GL_NO_ERROR == err ) {
                if ( i > 0 ) {
                    dalWarn(fmt::format("{} glError cleared.", i));
                }
                return;
            }
        }
    }

    void _logGLError(const int line, const char* const func, const char* const file) {
        for ( int i = 1; i <= 100; i++ ) {
            const auto err = glGetError();
            if ( GL_NO_ERROR == err ) return;

            LoggerGod::getinst().putWarn(gl_error_string(err), line, func, file);
        }

        clearGLError();
    }

}
