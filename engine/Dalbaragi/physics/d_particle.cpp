#include "d_particle.h"

#include <cassert>


// MassValue
namespace dal {

    float_t MassValue::getMassInv(void) const noexcept {
        return this->m_massInv;
    }

    std::optional<float_t> MassValue::getMass(void) const noexcept {
        if ( 0.0 == this->m_massInv ) {
            return std::nullopt;
        }
        else {
            return 1.0 / this->m_massInv;
        }
    }

    void MassValue::setMass(const float_t v) {
        assert(0.0 != v);
        this->m_massInv = 1.0 / v;
    }

    void MassValue::setMassInv(const float_t v) noexcept {
        this->m_massInv = v;
    }

}


namespace dal {

    constexpr float_t FREEZE_SPEED_THRESHOLD = 0.2;


    PositionParticle::PositionParticle(void) 
        : m_mass(1)
        , m_damping(0.9)
    {

    }

    void PositionParticle::addForce(const vec3_t force) {
        this->m_forceAccum += force;
    }

    void PositionParticle::integrate(const float_t deltaTime) {
        if ( this->m_mass.isInfinie() ) return;

        this->m_pos += this->m_vel * deltaTime;

        const auto acc = this->m_acc + this->m_forceAccum * this->m_mass.getMassInv();

        this->m_vel += acc * deltaTime;
        this->m_vel *= dal::pow(this->m_damping, deltaTime);
        if ( glm::length(this->m_vel) < FREEZE_SPEED_THRESHOLD ) {
            this->m_vel = vec3_t{ 0 };
        }

        this->m_forceAccum = vec3_t{ 0 };
    }

}
