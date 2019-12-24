#pragma once

#include <glm/glm.hpp>
#include <Fixed.h>


namespace dal {

    constexpr int FRAC_PART_SIZE = 7;
    using fixed_t = numeric::Fixed<32 - FRAC_PART_SIZE, FRAC_PART_SIZE>;

    using xvec2 = glm::tvec2<fixed_t>;
    using xvec3 = glm::tvec3<fixed_t>;
    using xvec4 = glm::tvec4<fixed_t>;

}
