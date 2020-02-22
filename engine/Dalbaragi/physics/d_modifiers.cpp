#include "d_modifiers.h"


namespace dal {

    ParticleGravity::ParticleGravity(void)
        : m_g(9.8)
    {

    }

    ParticleGravity::ParticleGravity(const float_t gravityAcc)
        : m_g(gravityAcc)
    {

    }

    void ParticleGravity::apply(const float_t deltaTime, PositionParticle& particle) {
        particle.addForce(vec3_t{ 0, -this->m_g, 0 });
    }


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

    constexpr float_t SPRING_MIN_REST_DIST = 0.001;


    FixedPointSpring::FixedPointSpring(void)
        : m_springConst(5)
        , m_restLen(2)
    {

    }

    FixedPointSpring::FixedPointSpring(float_t springConst, float_t restLen)
        : m_springConst(springConst)
        , m_restLen(restLen)
    {
        assert(this->m_restLen >= SPRING_MIN_REST_DIST);
    }

    void FixedPointSpring::apply(const float_t deltaTime, PositionParticle& fixed, PositionParticle& moving) {
        const auto fixedToMoving = moving.m_pos - fixed.m_pos;
        const auto distance = glm::length(fixedToMoving);
        if ( distance < SPRING_MIN_REST_DIST ) return;

        const auto magnitude = (this->m_restLen - distance) * this->m_springConst;
        const auto forceDirection = glm::normalize(fixedToMoving);
        moving.addForce(forceDirection * magnitude);
    }


    void FixedPointSpringPulling::apply(const float_t deltaTime, PositionParticle& fixed, PositionParticle& moving) {
        assert(this->m_restLen >= SPRING_MIN_REST_DIST);

        const auto fixedToMoving = moving.m_pos - fixed.m_pos;
        const auto distance = glm::length(fixedToMoving);
        if ( distance < this->m_restLen ) return;

        const auto magnitude = (this->m_restLen - distance) * this->m_springConst;
        const auto forceDirection = glm::normalize(fixedToMoving);
        moving.addForce(forceDirection * magnitude);
    }

}
