#include "d_modifiers.h"


namespace dal {

    FixedPointSpring::FixedPointSpring(void)
        : m_springConst(5)
        , m_restLen(1)
    {

    }

    void FixedPointSpring::apply(const float_t deltaTime, PositionParticle& fixed, PositionParticle& moving) {
        auto force = moving.m_pos;
        force -= fixed.m_pos;
        auto magnitude = glm::length(force);
        if ( magnitude < 0.0001 ) return;
        magnitude = (this->m_restLen - magnitude) * this->m_springConst;

        const auto forceDirection = glm::normalize(force);
        const auto forceResized = forceDirection * magnitude;
        moving.addForce(forceResized);
    }

}
