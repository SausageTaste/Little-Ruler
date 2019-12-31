#include "d_opengl_state.h"

#include <string>
#include <iostream>

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>

#include <d_shaderProcessor.h>
#include <d_filesystem.h>


namespace {

    auto glfunc(void) {
        return QOpenGLContext::currentContext()->extraFunctions();
    }

}


namespace dal::gl {

    void State::init(void) {
        dal::ShaderPreprocessor loader;

        {
            this->m_static.init(loader["r_static.vert"], loader["r_static.frag"]);
            this->u_static.init(this->m_static);
        }
    }


    void State::setViewportSize(const gl::int_t width, const gl::int_t height) {
        glfunc()->glViewport(0, 0, width, height);
        this->m_viewportSize.first = width;
        this->m_viewportSize.second = height;
    }


    const UniRender_Static& State::use_static(void) const {
        glfunc()->glEnable(GL_DEPTH_TEST);
        glfunc()->glEnable(GL_CULL_FACE);

        this->m_static.use();
        return this->u_static;
    }

}
