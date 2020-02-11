#include "d_particle.h"

#include <cassert>


#define DAL_USE_FIXED_DT true
#define DAL_FIXED_DELTA_TIME 1.0 / 30.0


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

#if DAL_USE_FIXED_DT
        constexpr float_t dt = DAL_FIXED_DELTA_TIME;
#else
        const float_t dt = deltaTime;
#endif

        this->m_pos += this->m_vel * dt;

        const auto acc = this->m_acc + this->m_forceAccum * this->m_mass.getMassInv();

        this->m_vel += acc * dt;
        this->m_vel *= dal::pow(this->m_damping, dt);

        this->m_forceAccum = vec3_t{ 0 };
    }

}
