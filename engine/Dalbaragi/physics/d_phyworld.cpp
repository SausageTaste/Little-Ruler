#include "d_phyworld.h"

#include "d_modifiers.h"


namespace dal {

    entt::entity PhysicsEntity::get(void) const {
        assert(this->m_id.has_value());
        return this->m_id.value();
    }

    entt::entity PhysicsEntity::operator*(void) const {
        return this->get();
    }

}


namespace dal {

    void PhysicsWorld::update(const float_t deltaTime) {
#if DAL_USE_FIXED_DT
        constexpr float_t dt = FIXED_DELTA_TIME;

        this->m_dtAccum += deltaTime;
        if ( this->m_dtAccum < FIXED_DELTA_TIME ) {
            return;
        }
        else {
            this->m_dtAccum = 0;
        }
#else
        const float_t dt = deltaTime;
#endif

        auto particleView = this->m_reg.view<PositionParticle>();

        // Apply modifiers
        {
            for ( auto& [modifier, entity] : this->m_unaryMod ) {
                auto& particle = this->m_reg.get<PositionParticle>(entity);
                modifier->apply(dt, particle);
            }

            for ( auto& [modifier, entities] : this->m_binaryMod ) {
                auto& one = this->m_reg.get<PositionParticle>(entities.first);
                auto& two = this->m_reg.get<PositionParticle>(entities.second);
                modifier->apply(dt, one, two);
            }

            static ParticleDrag drag;
            for ( const auto entity : particleView ) {
                auto& particle = particleView.get(entity);
                drag.apply(dt, particle);
            }
        }

        // Integrate
        {
            for ( const auto entity : particleView ) {
                auto& particle = particleView.get(entity);
                particle.integrate(dt);
            }
        }
    }

    PhysicsEntity PhysicsWorld::newEntity(void) {
        return PhysicsEntity{ this->m_reg.create() };
    }

    void PhysicsWorld::buildParticle(const PhysicsEntity& entity) {
        auto& posparticle = this->m_reg.assign<PositionParticle>(entity.get());
    }

    PositionParticle* PhysicsWorld::tryParticleOf(const PhysicsEntity& entity) {
        if ( this->m_reg.has<PositionParticle>(entity.get()) ) {
            return &this->m_reg.get<PositionParticle>(entity.get());
        }
        else {
            return nullptr;
        }
    }

    PositionParticle& PhysicsWorld::getParticleOf(const PhysicsEntity& entity) {
        auto result = this->tryParticleOf(entity);
        assert(nullptr != result);
        return *result;
    }

    void PhysicsWorld::registerUnaryMod(std::shared_ptr<UnaryPhyModifier> mod, const PhysicsEntity& particle) {
        this->m_unaryMod.emplace_back(mod, particle.get());
    }

    void PhysicsWorld::registerBinaryMod(std::shared_ptr<BinaryPhyModifier> mod, const PhysicsEntity& one, const PhysicsEntity& two) {
        this->m_binaryMod.emplace_back(mod, std::pair(one.get(), two.get()));
    }

}
