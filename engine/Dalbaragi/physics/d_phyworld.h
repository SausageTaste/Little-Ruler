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
        PhysicsEntity(void)
            : m_id(static_cast<entt::entity>(0))
        {

        }
        PhysicsEntity(const entt::entity id)
            : m_id(id)
        {

        }
      
        entt::entity get(void) const;
        entt::entity operator*(void) const;

    };


    class PhysicsWorld {

    private:
        using unaryModPair_t = std::pair< std::shared_ptr<UnaryPhyModifier>, entt::entity >;
        using binaryModPair_t = std::pair< std::shared_ptr<BinaryPhyModifier>, std::pair<entt::entity, entt::entity> >;

    private:
        entt::registry m_reg;

        std::vector<unaryModPair_t> m_unaryMod;
        std::vector<binaryModPair_t> m_binaryMod;

#if DAL_USE_FIXED_DT
        float_t m_dtAccum = 0;
#endif

    public:
        void update(const float_t deltaTime);

        PhysicsEntity newEntity(void);

        void buildParticle(const PhysicsEntity& entity);

        PhysicsEntity newParticle(void) {
            auto entity = this->newEntity();
            this->buildParticle(entity);
            return entity;
        }

        PositionParticle* tryParticleOf(const PhysicsEntity& entity);
        PositionParticle& getParticleOf(const PhysicsEntity& entity);

        void registerUnaryMod(std::shared_ptr<UnaryPhyModifier> mod, const PhysicsEntity& particle);
        void registerBinaryMod(std::shared_ptr<BinaryPhyModifier> mod, const PhysicsEntity& one, const PhysicsEntity& two);

    };

}
