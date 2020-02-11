#pragma once

#include <vector>
#include <memory>
#include <optional>

#include <entt/entity/registry.hpp>

#include "d_particle.h"
#include "d_modifierabc.h"


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
        using unaryModPair_t = std::pair< std::shared_ptr<UnaryPhyModifier>, entt::entity >;
        using binaryModPair_t = std::pair< std::shared_ptr<BinaryPhyModifier>, std::pair<entt::entity, entt::entity> >;

    private:
        entt::registry m_particles;

        std::vector<unaryModPair_t> m_unaryMod;
        std::vector<binaryModPair_t> m_binaryMod;

#if DAL_USE_FIXED_DT
        float_t m_dtAccum = 0;
#endif

    public:
        void update(const float_t deltaTime);

        ParticleEntity newParticleEntity(void);
        PositionParticle& getParticle(const ParticleEntity& entity);

        void registerUnaryMod(std::shared_ptr<UnaryPhyModifier> mod, const ParticleEntity& particle);
        void registerBinaryMod(std::shared_ptr<BinaryPhyModifier> mod, const ParticleEntity& one, const ParticleEntity& two);

    };

}
