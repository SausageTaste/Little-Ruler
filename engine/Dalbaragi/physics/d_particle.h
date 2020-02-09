#pragma once

#include <optional>

#include "d_precision.h"


namespace dal {

    class MassValue {

    private:
        float_t m_massInv = 0.0;

    public:
        MassValue(void) = default;

        float_t getMassInv(void) const noexcept;
        std::optional<float_t> getMass(void) const noexcept;

        void setMass(const float_t v);
        void setMassInv(const float_t v) noexcept;

    };


    class PositionParticle {

    public:
        vec3_t m_pos, m_vel, m_acc;
        MassValue m_mass;

    };

}
