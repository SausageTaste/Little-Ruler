#pragma once

#include <cmath>
#include <glm/glm.hpp>


namespace dal {

    using float_t = double;

    using vec2_t = glm::tvec2<float_t>;
    using vec3_t = glm::tvec3<float_t>;
    using vec4_t = glm::tvec4<float_t>;

    using mat4_t = glm::tmat4x4<float_t>;


    float_t pow(const float_t base, const float_t exponent);

}
