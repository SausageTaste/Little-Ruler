#pragma once

#include <glm/glm.hpp>
#include <Fixed.h>
#include <d_fixednum.h>


namespace dal {

    using fixed_t = dal::Fixed<100>;

    using xvec2 = glm::tvec2<fixed_t>;
    using xvec3 = glm::tvec3<fixed_t>;
    using xvec4 = glm::tvec4<fixed_t>;

}
