#include "d_phyworld.h"


namespace dal {

    PhysicsEntity::PhysicsEntity(PhysicsEntity&& other) noexcept
        : m_id(std::move(other.m_id))
    {
        other.m_id.reset();
    }

    PhysicsEntity& PhysicsEntity::operator=(PhysicsEntity&& other) noexcept {
        this->m_id = std::move(other.m_id);
        other.m_id.reset();
        return *this;
    }

    entt::entity PhysicsEntity::get(void) const {
        assert(this->m_id.has_value());
        return this->m_id.value();
    }

    entt::entity PhysicsEntity::operator*(void) const {
        return this->get();
    }

}


namespace dal {

    ParticleEntity PhysicsWorld::newParticleEntity(void) {
        ParticleEntity entity{ this->m_particles.create(), this->m_particles };
        auto& posparticle = this->m_particles.assign<PositionParticle>(entity.get());
        return entity;
    }

    PositionParticle& PhysicsWorld::getParticle(const ParticleEntity& entity) {
        assert(this->m_particles.has<PositionParticle>(entity.get()));
        return this->m_particles.get<PositionParticle>(entity.get());
    }

}
