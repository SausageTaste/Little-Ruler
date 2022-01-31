#pragma once

#include <optional>

#include "d_precision.h"


namespace dal {

    class MassValue {

    private:
        float_t m_massInv = 0.0;

    public:
        MassValue(void) = default;
        MassValue(const float_t mass) {
            this->setMass(mass);
        }

        float_t getMassInv(void) const noexcept;
        std::optional<float_t> getMass(void) const noexcept;

        void setMass(const float_t v);
        void setMassInv(const float_t v) noexcept;

        bool isInfinie(void) const noexcept {
            return this->getMassInv() <= static_cast<float_t>(0);
        }

    };


    class PositionParticle {

    public:
        vec3_t m_pos{};
        MassValue m_mass;
        float_t m_damping;

    private:
        vec3_t m_vel{}, m_acc{}, m_forceAccum{};

    public:
        PositionParticle(void);

        const vec3_t& velocity(void) const {
            return this->m_vel;
        }

        void addForce(const vec3_t force);
        void integrate(const float_t deltaTime);

    };

}
