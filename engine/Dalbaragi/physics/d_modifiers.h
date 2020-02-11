#pragma once

#include "d_modifierabc.h"


namespace dal {

    class FixedPointSpring : public BinaryPhyModifier {

    private:
        float_t m_springConst, m_restLen;

    public:
        FixedPointSpring(void);
        virtual void apply(const float_t deltaTime, PositionParticle& fixed, PositionParticle& moving) override;

    };

}
