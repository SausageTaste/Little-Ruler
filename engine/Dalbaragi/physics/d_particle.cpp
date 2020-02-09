#include "d_particle.h"

#include <cassert>


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
