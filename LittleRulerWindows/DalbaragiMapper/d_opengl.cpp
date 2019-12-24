#include "d_opengl.h"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>


namespace {

    static_assert(std::is_same< GLint, dal::gl::int_t >::value);
    static_assert(std::is_same< GLuint, dal::gl::uint_t >::value);
    static_assert(std::is_same< GLfloat, dal::gl::float_t >::value);


    auto glfunc(void) {
        return QOpenGLContext::currentContext()->extraFunctions();
    }

}


namespace dal::gl {

    void setClearColor(const float r, const float g, const float b, const float a) {
        glfunc()->glClearColor(r, g, b, a);
    }

    void clear(const ClearMode mode) {
        switch ( mode ) {

        case ClearMode::color:
            glfunc()->glClear(GL_COLOR_BUFFER_BIT); break;
        case ClearMode::depth:
            glfunc()->glClear(GL_DEPTH_BUFFER_BIT); break;
        case ClearMode::color_depth:
            glfunc()->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); break;
        default:
            abort();

        }
    }


    std::pair<gl::int_t, gl::int_t> queryVersion(void) {
        std::pair<gl::int_t, gl::int_t> result;

        auto f = glfunc();
        f->glGetIntegerv(GL_MAJOR_VERSION, &result.first);
        f->glGetIntegerv(GL_MINOR_VERSION, &result.second);

        return result;
    }

    std::string queryRendererName(void) {
        auto a = glfunc()->glGetString(GL_RENDERER);
        return reinterpret_cast<const char*>(a);
    }

}
