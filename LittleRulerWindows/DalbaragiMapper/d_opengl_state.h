#pragma once

#include <glm/glm.hpp>

#include "d_uniloc.h"


namespace dal::gl {

    class State {

    private:
        std::pair<gl::int_t, gl::int_t> m_viewportSize;

        Shader m_static;
        UniRender_Static u_static;

    public:
        void init(void);

        void setViewportSize(const gl::int_t width, const gl::int_t height);
        const std::pair<gl::int_t, gl::int_t>& viewportSize(void) const{
            return this->m_viewportSize;
        }

        const UniRender_Static& use_static(void) const;

    };

}
