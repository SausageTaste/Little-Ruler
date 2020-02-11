#include "d_modifiers.h"


namespace dal {

    ParticleDrag::ParticleDrag(void)
        : m_k1(0.5)
        , m_k2(0.5)
    {

    }

    void ParticleDrag::apply(const float_t deltaTime, PositionParticle& particle) {
        const auto force = particle.velocity();
        const auto length = glm::length(force);
        if ( length < 0.0001 ) return;
        const auto dragCoeff = -(this->m_k1 * length + this->m_k2 * length * length);

        particle.addForce(dragCoeff / length * force);
    }

}


namespace dal {

    FixedPointSpring::FixedPointSpring(void)
        : m_springConst(5)
        , m_restLen(2)
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
