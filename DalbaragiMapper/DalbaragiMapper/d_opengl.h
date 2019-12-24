#pragma once

#include <string>


// Definitions
namespace dal::gl {

    using int_t = int;
    using uint_t = unsigned;
    using float_t = float;

    enum class Type { gl_float, gl_int, gl_uint };

    enum class ClearMode { color, depth, color_depth };

}


// Funcs
namespace dal::gl {

    void setClearColor(const float r, const float g, const float b, const float a);
    void clear(const ClearMode mode);

    std::pair<gl::int_t, gl::int_t> queryVersion(void);
    std::string queryRendererName(void);

}
