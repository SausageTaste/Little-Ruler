#pragma once

#include "d_modifierabc.h"


namespace dal {

    class ParticleGravity : public UnaryPhyModifier {

    private:
        float_t m_g;

    public:
        ParticleGravity(void);
        ParticleGravity(const float_t gravityAcc);

        virtual void apply(const float_t deltaTime, PositionParticle& particle) override;

    };

    class ParticleDrag : public UnaryPhyModifier {

    private:
        float_t m_k1, m_k2;

    public:
        ParticleDrag(void);
        virtual void apply(const float_t deltaTime, PositionParticle& particle) override;

    };


    class FixedPointSpring : public BinaryPhyModifier {

    protected:
        float_t m_springConst, m_restLen;

    public:
        FixedPointSpring(void);
        FixedPointSpring(float_t springConst, float_t restLen);

        virtual void apply(const float_t deltaTime, PositionParticle& fixed, PositionParticle& moving) override;

    };

    class FixedPointSpringPulling : public FixedPointSpring {

    public:
        using FixedPointSpring::FixedPointSpring;

        virtual void apply(const float_t deltaTime, PositionParticle& fixed, PositionParticle& moving) override;

    };

}
