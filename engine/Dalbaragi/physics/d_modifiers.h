#pragma once

#include "d_modifierabc.h"


namespace dal {

    class ParticleDrag : public UnaryPhyModifier {

    private:
        float_t m_k1, m_k2;

    public:
        ParticleDrag(void);
        virtual void apply(const float_t deltaTime, PositionParticle& particle) override;

    };


    class FixedPointSpring : public BinaryPhyModifier {

    private:
        float_t m_springConst, m_restLen;

    public:
        FixedPointSpring(void);
        virtual void apply(const float_t deltaTime, PositionParticle& fixed, PositionParticle& moving) override;

    };

}
