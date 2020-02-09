#pragma once

#include <optional>

#include <entt/entity/registry.hpp>

#include "d_particle.h"


namespace dal {

    class PhysicsEntity {

    private:
        std::optional<entt::entity> m_id;

    public:
        PhysicsEntity(const PhysicsEntity&) = delete;
        PhysicsEntity& operator=(const PhysicsEntity&) = delete;

    public:
        PhysicsEntity(void)
            : m_id(static_cast<entt::entity>(0))
        {

        }
        PhysicsEntity(const entt::entity id, entt::registry& registry)
            : m_id(id)
        {

        }
        PhysicsEntity(PhysicsEntity&&) noexcept;
        PhysicsEntity& operator=(PhysicsEntity&&) noexcept;

        entt::entity get(void) const;
        entt::entity operator*(void) const;

    };

    class ParticleEntity : public PhysicsEntity {
    
    public:
        using PhysicsEntity::PhysicsEntity;

    };


    class PhysicsWorld {

    private:
        entt::registry m_particles;

    public:
        ParticleEntity newParticleEntity(void);
        PositionParticle& getParticle(const ParticleEntity& entity);

    };

}
