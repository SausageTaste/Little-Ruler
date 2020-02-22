#include "p_scene.h"

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>

#include <d_logger.h>
#include <d_modifiers.h>

#include "s_configs.h"
#include "g_charastate.h"
#include "u_math.h"


using namespace fmt::literals;


namespace {

    void bindCameraPos(dal::StrangeEulerCamera& camera, const glm::vec3 thisPos, const glm::vec3 lastPos) {
        // Apply move direction
        {
            const glm::vec3 MODEL_ORIGIN_OFFSET{ 0.0f, 1.3f, 0.0f };
            constexpr float MAX_Y_DEGREE = 75.0f;
            constexpr float CAM_ROTATE_SPEED_INV = 1.0f;
            static_assert(0.0f <= CAM_ROTATE_SPEED_INV && CAM_ROTATE_SPEED_INV <= 1.0f);

            const auto camOrigin = thisPos + MODEL_ORIGIN_OFFSET;

            {
                const auto deltaPos = thisPos - lastPos;
                camera.m_pos += deltaPos * CAM_ROTATE_SPEED_INV;
            }

            {
                const auto obj2CamVec = camera.m_pos - camOrigin;
                const auto len = glm::length(obj2CamVec);
                auto obj2CamSEuler = dal::vec2StrangeEuler(obj2CamVec);

                obj2CamSEuler.clampY(glm::radians(-MAX_Y_DEGREE), glm::radians(MAX_Y_DEGREE));
                const auto rotatedVec = dal::strangeEuler2Vec(obj2CamSEuler);
                camera.m_pos = camOrigin + rotatedVec * len;
            }

            {
                constexpr float OBJ_CAM_DISTANCE = 3.0f;

                const auto cam2ObjVec = camOrigin - camera.m_pos;
                const auto cam2ObjSEuler = dal::vec2StrangeEuler(cam2ObjVec);
                camera.setViewPlane(cam2ObjSEuler.getX(), cam2ObjSEuler.getY());

                camera.m_pos = camOrigin - dal::resizeOnlyXZ(cam2ObjVec, OBJ_CAM_DISTANCE);
            }
        }

        camera.updateViewMat();
    }

}


// Modifiers
namespace {

    class EntityToParticle : public dal::IEntityController {

    private:
        dal::PhysicsWorld& m_phyworld;
        dal::PhysicsEntity m_particleEntt;

    public:
        EntityToParticle(const dal::PhysicsEntity& particleEntt, dal::PhysicsWorld& phyworld)
            : m_phyworld(phyworld)
            , m_particleEntt(particleEntt)
        {

        }

        virtual void apply(const entt::entity entity, entt::registry& reg) override {
            auto& particle = this->m_phyworld.getParticleOf(this->m_particleEntt);
            auto& trans = reg.get<dal::cpnt::Transform>(entity);
            trans.setPos(particle.m_pos);
        }

    };

    class ParticleToEntity : public dal::UnaryPhyModifier {

    private:
        entt::entity m_entity;
        entt::registry& m_registry;
        glm::mat4 m_offset;

    public:
        ParticleToEntity(const entt::entity entity, entt::registry& registry)
            : m_entity(entity)
            , m_registry(registry)
            , m_offset(1.f)
        {

        }
        ParticleToEntity(const entt::entity entity, entt::registry& registry, const glm::mat4& offset)
            : m_entity(entity)
            , m_registry(registry)
            , m_offset(offset)
        {

        }

        virtual void apply(const dal::float_t deltaTime, dal::PositionParticle& particle) override {
            auto& trans = this->m_registry.get<dal::cpnt::Transform>(this->m_entity);
            particle.m_pos = trans.getMat() * this->m_offset * glm::vec4{ 0, 0, 0, 1 };
        }

    };


    class HairJointPhysics : public dal::IJointModifier {

    private:
        dal::PhysicsWorld& m_phyworld;
        dal::PhysicsEntity m_particleEntt;

        entt::entity m_entity;
        entt::registry& m_registry;

    public:
        HairJointPhysics(const dal::PhysicsEntity& particleEntt, dal::PhysicsWorld& phyworld, const entt::entity entity, entt::registry& registry)
            : m_phyworld(phyworld)
            , m_particleEntt(particleEntt)
            , m_entity(entity)
            , m_registry(registry)
        {

        }

        virtual glm::mat4 makeTransform(const float elapsed, const dal::jointID_t jid, const dal::SkeletonInterface& skeleton) override {
            auto& particle = this->m_phyworld.getParticleOf(this->m_particleEntt);
            auto& trans = this->m_registry.get<dal::cpnt::Transform>(this->m_entity);
            const auto& jointInfo = skeleton.at(jid);
            const auto decomposed = dal::decomposeTransform(jointInfo.offsetInv());

            const auto jointToParticle = glm::vec3{ particle.m_pos } -(decomposed.first + trans.getPos());
            return glm::translate(glm::mat4{ 1.f }, jointToParticle);
        }

    };

}


// Hair
namespace {

    class HairMaster {

    private:
        dal::PhysicsWorld m_phyworld;

    public:
        HairMaster(const entt::entity entity, entt::registry& reg) {
            auto& animModel = reg.get<dal::cpnt::AnimatedModel>(entity);
            const auto& trans = reg.get<dal::cpnt::Transform>(entity);
            const auto& skeleton = animModel.m_model->getSkeletonInterf();
        }

    private:
        static dal::jointID_t findHairRoot(const dal::SkeletonInterface& skeleton) {
            for ( int i = 0; i < skeleton.getSize(); ++i ) {
                if ( 1 == skeleton.at(i).jointType() ) {
                    return i;
                }
            }

            dalAbort("Failed to find hair root joint.");
        }

    };

}


// SceneGraph
namespace dal {

    SceneGraph::SceneGraph(ResourceMaster& resMas, PhysicsWorld& phyworld, const unsigned int winWidth, const unsigned int winHeight)
        : m_resMas(resMas)
        , m_phyworld(phyworld)
    {
        // This is needed by Water objects
        {
            GlobalStateGod::getinst().setWinSize(winWidth, winHeight);
        }

        //this->loadMap("asset::map/water_bowl.dlb");

        auto map = this->m_resMas.loadMap("asset::map/water_n_slope.dlb");
        this->m_mapChunks2.push_back(std::move(map));

        // Player
        {
            this->m_player = this->m_entities.create();
            dalInfo(fmt::format("Player's entity id is {}.", this->m_player));

            auto& transform = this->m_entities.assign<cpnt::Transform>(this->m_player);
            //transform.setScale(1.5f);

            auto ptrModel = this->m_resMas.orderModelAnim("asset::irin.dmd");
            auto& renderable = this->m_entities.assign<cpnt::AnimatedModel>(this->m_player);
            renderable.m_model = ptrModel;

            this->m_entities.assign<cpnt::CharacterState>(this->m_player, transform, renderable, this->m_playerCam, *this);

            this->m_entities.assign<cpnt::PhysicsObj>(this->m_player);
        }

        // Test physics
        {
            auto playerParticle = this->m_phyworld.newParticle();
            this->m_phyworld.registerUnaryMod(
                std::shared_ptr<dal::UnaryPhyModifier>{new ParticleToEntity{ this->m_player, this->m_entities }},
                playerParticle
            );

            const auto entity = this->addObj_static("asset::pbr_ball.dmd");
            auto& transform = this->m_entities.get<cpnt::Transform>(entity);
            transform.setScale(0.6);

            auto enttParticle = this->m_phyworld.newParticle();
            auto& particle = this->m_phyworld.getParticleOf(enttParticle);
            particle.m_pos = glm::vec3{ 0, 3, 0 };
            this->m_phyworld.registerBinaryMod(
                std::shared_ptr<BinaryPhyModifier>{ new FixedPointSpringPulling{ 5, 3 } },
                playerParticle, enttParticle
            );

            auto& enttCtrl = this->m_entities.assign<cpnt::EntityCtrl>(entity);
            enttCtrl.m_ctrler.reset(new EntityToParticle{ enttParticle, this->m_phyworld });
        }
    }


    void SceneGraph::update(const float deltaTime) {
        // Apply entity controllers
        {
            auto view = this->m_entities.view<cpnt::EntityCtrl>();
            for ( const auto entity : view ) {
                auto& enttCtrl = view.get(entity);
                enttCtrl.m_ctrler->apply(entity, this->m_entities);
            }
        }

        // Resolve collisions
        {
            auto& trans = this->m_entities.get<cpnt::Transform>(this->m_player);
            auto& model = this->m_entities.get<cpnt::AnimatedModel>(this->m_player);

            const auto bounding = model.m_model->getBounding();
            if ( nullptr != bounding ) {
                const auto lastPos = trans.getPos();
                this->applyCollision(*bounding, trans);
                bindCameraPos(this->m_playerCam, trans.getPos(), lastPos);
            }
        }

        // Update animtions of dynamic objects.
        {
            auto view = this->m_entities.view<cpnt::AnimatedModel>();
            for ( const auto entity : view ) {
                auto& cpntModel = view.get(entity);
                auto pModel = cpntModel.m_model;
                updateAnimeState(cpntModel.m_animState, pModel->getAnimations(), pModel->getSkeletonInterf());
            }
        }
    }


    entt::entity SceneGraph::addObj_static(const char* const resid) {
        const auto entity = this->m_entities.create();

        auto& transform = this->m_entities.assign<cpnt::Transform>(entity);

        auto ptrModel = this->m_resMas.orderModelStatic(resid);
        auto& renderable = this->m_entities.assign<cpnt::StaticModel>(entity);
        renderable.m_model = ptrModel;

        return entity;
    }


    void SceneGraph::renderWater(const UnilocWaterry& uniloc) {
        for ( auto& map : m_mapChunks2 ) {
            map.renderWater(uniloc);
        }
    }


    void SceneGraph::render_static(const UniRender_Static& uniloc) {
        for ( auto& map : this->m_mapChunks2 ) {
            map.render_static(uniloc);
        }

        const auto view = this->m_entities.view<cpnt::Transform, cpnt::StaticModel>();
        for ( const auto entity : view ) {
            auto& cpntTrans = view.get<cpnt::Transform>(entity);
            auto& cpntModel = view.get<cpnt::StaticModel>(entity);

            uniloc.modelMat(cpntTrans.getMat());
            cpntModel.m_model->render(uniloc);
        }
    }

    void SceneGraph::render_animated(const UniRender_Animated& uniloc) {
        for ( auto& map : this->m_mapChunks2 ) {
            map.render_animated(uniloc);
        }

        const auto viewAnimated = this->m_entities.view<cpnt::Transform, cpnt::AnimatedModel>();
        for ( const auto entity : viewAnimated ) {
            auto& cpntTrans = viewAnimated.get<cpnt::Transform>(entity);
            auto& cpntModel = viewAnimated.get<cpnt::AnimatedModel>(entity);

            uniloc.modelMat(cpntTrans.getMat());
            cpntModel.m_model->render(uniloc, cpntModel.m_animState.getTransformArray());
        }
    }

    void SceneGraph::render_staticDepth(const UniRender_StaticDepth& uniloc) {
        for ( auto& map : this->m_mapChunks2 ) {
            map.render_staticDepth(uniloc);
        }

        this->m_entities.view<cpnt::Transform, cpnt::StaticModel>().each(
            [&uniloc](const cpnt::Transform& trans, const cpnt::StaticModel& model) {
                uniloc.modelMat(trans.getMat());
                model.m_model->render(uniloc);
            }
        );
    }

    void SceneGraph::render_animatedDepth(const UniRender_AnimatedDepth& uniloc) {
        const auto viewAnimated = this->m_entities.view<cpnt::Transform, cpnt::AnimatedModel>();
        for ( const auto entity : viewAnimated ) {
            auto& cpntTrans = viewAnimated.get<cpnt::Transform>(entity);
            auto& cpntModel = viewAnimated.get<cpnt::AnimatedModel>(entity);

            uniloc.modelMat(cpntTrans.getMat());
            cpntModel.m_model->render(uniloc, cpntModel.m_animState.getTransformArray());
        }
    }


    std::vector<WaterRenderer*> SceneGraph::waters(void) {
        std::vector<WaterRenderer*> result;

        for ( auto& map : this->m_mapChunks2 ) {
            map.getWaters(result);
        }

        return result;
    }


    void SceneGraph::applyCollision(const ICollider& inCol, cpnt::Transform& inTrans) {
        for ( auto& map : this->m_mapChunks2 ) {
            map.applyCollision(inCol, inTrans);
        }
    }

    std::optional<RayCastingResult> SceneGraph::doRayCasting(const Segment& ray) {
        std::optional<RayCastingResult> result{ std::nullopt };
        float closestDist = std::numeric_limits<float>::max();

        for ( auto& map : this->m_mapChunks2 ) {
            auto info = map.castRayToClosest(ray);
            if ( info ) {
                if ( info->m_distance < closestDist ) {
                    result = info;
                    closestDist = info->m_distance;
                }
            }
        }

        return result;
    }

    void SceneGraph::onResize(const unsigned int width, const unsigned int height) {
        for ( auto& map : m_mapChunks2 ) {
            map.onWinResize(width, height);
        }
    }

}
