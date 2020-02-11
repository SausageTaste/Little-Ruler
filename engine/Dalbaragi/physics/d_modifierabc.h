#pragma once

#include "d_particle.h"


namespace dal {

    class UnaryPhyModifier {

    public:
        virtual ~UnaryPhyModifier(void) = default;
        virtual void apply(const float_t deltaTime, PositionParticle& particle) = 0;

    };

    class BinaryPhyModifier {

    public:
        virtual ~BinaryPhyModifier(void) = default;
        virtual void apply(const float_t deltaTime, PositionParticle& one, PositionParticle& two) = 0;

    };

}
