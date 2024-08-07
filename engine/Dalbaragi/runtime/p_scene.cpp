#include "p_scene.h"

#include <limits>

#include <spdlog/fmt/fmt.h>
#include <glm/gtc/matrix_transform.hpp>

#include <d_logger.h>
#include <d_modifiers.h>
#include <d_filesystem.h>
#include <d_mapparser.h>
#include <d_debugview.h>

#include "s_configs.h"
#include "g_charastate.h"
#include "u_math.h"


using namespace fmt::literals;


namespace {

    const dal::ColAABB PLAYER_AABB{ glm::vec3{-0.3, 0.3, -0.3}, glm::vec3{0.3, 1.3, 0.3} };


    void bindCameraPos(dal::FPSEulerCamera& camera, const glm::vec3 thisPos, const glm::vec3 lastPos) {
        // Apply move direction
        {
            const glm::vec3 MODEL_ORIGIN_OFFSET{ 0.0f, 1.3f, 0.0f };
            constexpr float MAX_Y_DEGREE = 75.0f;
            constexpr float CAM_ROTATE_SPEED_INV = 1.0f;
            static_assert(0.0f <= CAM_ROTATE_SPEED_INV && CAM_ROTATE_SPEED_INV <= 1.0f);

            const auto camOrigin = thisPos + MODEL_ORIGIN_OFFSET;

            {
                const auto deltaPos = thisPos - lastPos;
                camera.addPos(deltaPos * CAM_ROTATE_SPEED_INV);
            }

            {
                const auto obj2CamVec = camera.pos() - camOrigin;
                const auto len = glm::length(obj2CamVec);
                auto obj2CamSEuler = dal::vec2fpsEuler(obj2CamVec);

                obj2CamSEuler.clampY(glm::radians(-MAX_Y_DEGREE), glm::radians(MAX_Y_DEGREE));
                const auto rotatedVec = dal::fpsEuler2vec(obj2CamSEuler);
                camera.setPos(camOrigin + rotatedVec * len);
            }

            {
                constexpr float OBJ_CAM_DISTANCE = 3.0f;

                const auto cam2ObjVec = camOrigin - camera.pos();
                const auto cam2ObjSEuler = dal::vec2fpsEuler(cam2ObjVec);
                camera.setViewPlane(cam2ObjSEuler.x(), cam2ObjSEuler.y());

                camera.setPos(camOrigin - dal::resizeOnlyXZ(cam2ObjVec, OBJ_CAM_DISTANCE));
            }
        }

        camera.updateViewMat();
    }

    bool isGoodToBeLoaded(const glm::vec3& campos, const glm::vec3& camdirec, const dal::LevelData::ChunkData& mapInfo) {
        if ( mapInfo.m_aabb.isInside(campos) )
            return true;

        dal::Segment seg{ campos + glm::vec3{0, 500, 0} , glm::vec3{0, -1000, 0} };
        if ( dal::isIntersecting(seg, mapInfo.m_aabb) )
            return true;

        const auto camdirec_n = glm::normalize(camdirec);
        for ( const auto& p : mapInfo.m_aabb.vertices() ) {
            const auto toVert = glm::normalize(p - campos);
            const auto dot = glm::dot(toVert, camdirec_n);
            if ( dot > std::cos(glm::radians<float>(45)) )
                return true;
        }

        {
            double closestDist = std::numeric_limits<double>::max();

            for ( const auto& p : mapInfo.m_aabb.vertices() ) {
                const double dist = glm::distance(p, campos);
                if ( dist < closestDist ) {
                    closestDist = dist;
                }
            }

            return closestDist < 10.0;
        }
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
        class HairPullingForce : public dal::BinaryPhyModifier {

        private:
            dal::float_t m_restLen;

        public:
            HairPullingForce(const dal::float_t restLen)
                : m_restLen(restLen)
            {

            }

            virtual void apply(const dal::float_t deltaTime, dal::PositionParticle& one, dal::PositionParticle& two) override {
                const auto one2two = two.m_pos - one.m_pos;
                const auto dist = glm::length(one2two);

                if ( dist > this->m_restLen ) {
                    const auto one2two_n = one2two / dist;
                    const auto newOffset = one2two_n * this->m_restLen;
                    two.m_pos = one.m_pos + newOffset;
                }
            }

        };

        class HairStructure {

        };

    private:
        dal::PhysicsWorld m_phyworld;

        const entt::entity m_targetEntity;
        entt::registry& m_reg;

    public:
        HairMaster(const entt::entity entity, entt::registry& reg)
            : m_targetEntity(entity)
            , m_reg(reg)
        {

        }

        void update(void) {
            auto& animModel = this->m_reg.get<dal::cpnt::AnimatedModel>(this->m_targetEntity);
            const auto& trans = this->m_reg.get<dal::cpnt::Transform>(this->m_targetEntity);
            const auto& skeleton = animModel.m_model->getSkeletonInterf();
        }

    private:
        static dal::jointID_t findHairRoot(const dal::SkeletonInterface& skeleton) {
            for ( int i = 0; i < skeleton.getSize(); ++i ) {
                if ( dal::JointType::hair_root == skeleton.at(i).jointType() ) {
                    return i;
                }
            }

            dalAbort("Failed to find hair root joint.");
        }

    };

    std::unique_ptr<HairMaster> g_hairMas;

}


// LevelData
namespace dal {

    LevelData::ChunkData& LevelData::newChunk(void) {
        return this->m_chunks.emplace_back();
    }

    void LevelData::clear(void) {
        this->m_chunks.clear();
    }

    size_t LevelData::size(void) const {
        return this->m_chunks.size();
    }

    void LevelData::reserve(const size_t s) {
        this->m_chunks.reserve(s);
    }

    void LevelData::setRespath(const std::string& respath) {
        this->m_respath = respath;
    }

}

#include <iostream>
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

        // Old map
        {
            //auto map = this->m_resMas.loadMap("asset::map/water_n_slope.dlb");
            //this->m_mapChunks2.push_back(std::move(map));
        }

        // New map
        {
            this->openLevel("asset::demo_dal_map.dlb");
        }

        // Player
        {
            this->m_player = this->m_entities.create();
            dalInfo(fmt::format("Player's entity id is {}.", static_cast<uint32_t>(this->m_player)));

            auto& transform = this->m_entities.assign<cpnt::Transform>(this->m_player);
            transform.setScale(0.2f);

            auto ptrModel = this->m_resMas.orderModelAnim("asset::Character Running.dmd");
            auto& renderable = this->m_entities.assign<cpnt::AnimatedModel>(this->m_player);
            renderable.m_model = ptrModel;

            this->m_entities.assign<cpnt::CharacterState>(this->m_player, transform, renderable, this->m_playerCam, *this);

            this->m_entities.assign<cpnt::PhysicsObj>(this->m_player);

            g_hairMas.reset(new HairMaster{ this->m_player, this->m_entities });
        }

        // Test physics
        /*{
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
        }*/

        // Misc
        {
            this->m_playerCam.setPos(0, 0, -3);
            this->m_playerCam.setFocusPoint(0);
            this->m_playerCam.updateViewMat();
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
            constexpr unsigned MAX_ITERATION = 3;

            auto& trans = this->m_entities.get<cpnt::Transform>(this->m_player);
            auto playerAABB = PLAYER_AABB.transform(trans.getPos(), trans.getScale());
            const auto deltaPlayerMove = trans.getPos() - this->m_playerLastTrans.getPos();

            std::vector<dal::AABB> aabbs;
            std::vector<dal::Triangle> triangles;

            // Get sets of colliders
            {
                dal::TriangleSorter triangleHeap{ -deltaPlayerMove };
                for ( const auto& map : this->m_mapChunks )
                    if ( dal::isIntersecting(playerAABB, map.m_info->m_aabb) )
                        map.m_map.findIntersctionsToStatic(playerAABB, aabbs, triangleHeap);

                const auto numTri = triangleHeap.size();
                for ( size_t i = 0; i < numTri; ++i ) {
                    triangles.push_back(triangleHeap.pop());
                }
            }

            // Resolve AABBs
            for ( const auto& aabb : aabbs ) {
                const auto result = dal::calcResolveForAABB(playerAABB, aabb);
                trans.addPos(result);
                playerAABB = PLAYER_AABB.transform(trans.getPos(), trans.getScale());
            }

            // Resolve triangles
            for ( int i = 0; i < MAX_ITERATION; ++i ) {
                bool resolved = false;

                for ( const auto& tri : triangles ) {
                    if ( dal::isIntersecting(tri, playerAABB) ) {
                        const auto [dist, direc] = dal::calcIntersectingDepth(playerAABB, tri.plane());
                        if ( 0.f == dist )
                            continue;

                        trans.addPos(-dist * direc);
                        playerAABB = PLAYER_AABB.transform(trans.getPos(), trans.getScale());
                        resolved = true;
                    }
                }

                if ( !resolved )
                    break;
            }

            // Draw player aabb
            for ( auto& tri : playerAABB.makeTriangles() )
                dal::DebugViewGod::inst().addTriangle(tri.point0(), tri.point1(), tri.point2(), glm::vec4{ 0, 1, 0, 0.2 });

            this->m_playerLastTrans = trans;
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

        // Find map chunks to load
        for ( unsigned i = 0; i < this->m_activeLevel.size(); ++i ) {
            auto& mapInfo = this->m_activeLevel.at(i);
            if ( mapInfo.m_active )
                continue;

            if ( ::isGoodToBeLoaded(this->m_playerCam.pos(), this->m_playerCam.direction(), mapInfo) ) {
                const auto respath = parseResPath(this->m_activeLevel.respath());
                const auto chunkPath = respath.m_package + "::" + respath.m_intermPath + mapInfo.m_name + ".dmc";
                this->openChunk(chunkPath.c_str(), mapInfo);
                mapInfo.m_active = true;
                dalInfo(fmt::format("Map chunk activated: {}", mapInfo.m_name));
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


    void SceneGraph::render_static(const UniRender_Static& uniloc) {
        this->sendDlightUniform(uniloc.i_lighting);

        for ( auto& map : this->m_mapChunks ) {
            map.m_map.render_static(uniloc);
        }

        const auto view = this->m_entities.view<cpnt::Transform, cpnt::StaticModel>();
        for ( const auto entity : view ) {
            auto& cpntTrans = view.get<cpnt::Transform>(entity);
            auto& cpntModel = view.get<cpnt::StaticModel>(entity);

            auto envmap = this->findClosestEnv(cpntTrans.getPos());
            if ( nullptr != envmap )
                sendEnvmapUniform(*envmap, uniloc.i_envmap);
            else
                uniloc.i_envmap.hasEnvmap(false);

            auto map = this->findClosestMapChunk(cpntTrans.getPos());
            if ( nullptr != map ) {
                map->sendPlightUniforms(uniloc.i_lighting);
                map->sendSlightUniforms(uniloc.i_lighting);
            }
            else {
                uniloc.i_lighting.plightCount(0);
                uniloc.i_lighting.slightCount(0);
            }

            uniloc.modelMat(cpntTrans.getMat());
            cpntModel.m_model->render(uniloc);
        }
    }

    void SceneGraph::render_animated(const UniRender_Animated& uniloc) {
        this->sendDlightUniform(uniloc.i_lighting);

        const auto viewAnimated = this->m_entities.view<cpnt::Transform, cpnt::AnimatedModel>();
        for ( const auto entity : viewAnimated ) {
            auto& cpntTrans = viewAnimated.get<cpnt::Transform>(entity);
            auto& cpntModel = viewAnimated.get<cpnt::AnimatedModel>(entity);

            auto envmap = this->findClosestEnv(cpntTrans.getPos());
            if ( nullptr != envmap )
                sendEnvmapUniform(*envmap, uniloc.i_envmap);
            else
                uniloc.i_envmap.hasEnvmap(false);

            auto map = this->findClosestMapChunk(cpntTrans.getPos());
            if ( nullptr != map ) {
                map->sendPlightUniforms(uniloc.i_lighting);
                map->sendSlightUniforms(uniloc.i_lighting);
            }
            else {
                uniloc.i_lighting.plightCount(0);
                uniloc.i_lighting.slightCount(0);
            }

            uniloc.modelMat(cpntTrans.getMat());
            cpntModel.m_model->render(uniloc, cpntModel.m_animState.getTransformArray());
        }
    }

    void SceneGraph::render_staticDepth(const UniRender_StaticDepth& uniloc) {
        for ( auto& map : this->m_mapChunks ) {
            map.m_map.render_staticDepth(uniloc);
        }

        this->m_entities.view<cpnt::Transform, cpnt::StaticModel>().each(
            [&uniloc](const cpnt::Transform& trans, const cpnt::StaticModel& model) {
                uniloc.modelMat(trans.getMat());
                model.m_model->render();
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

    void SceneGraph::render_staticOnWater(const UniRender_StaticOnWater& uniloc) {
        this->sendDlightUniform(uniloc.i_lighting);

        for ( auto& map : this->m_mapChunks ) {
            map.m_map.render_staticOnWater(uniloc);
        }

        const auto view = this->m_entities.view<cpnt::Transform, cpnt::StaticModel>();
        for ( const auto entity : view ) {
            auto& cpntTrans = view.get<cpnt::Transform>(entity);
            auto& cpntModel = view.get<cpnt::StaticModel>(entity);

            uniloc.modelMat(cpntTrans.getMat());
            cpntModel.m_model->render(uniloc);
        }
    }

    void SceneGraph::render_animatedOnWater(const UniRender_AnimatedOnWater& uniloc) {
        this->sendDlightUniform(uniloc.i_lighting);

        if ( !this->m_mapChunks.empty() ) {
            this->m_mapChunks.back().m_map.sendPlightUniforms(uniloc.i_lighting);
            this->m_mapChunks.back().m_map.sendSlightUniforms(uniloc.i_lighting);
        }

        const auto viewAnimated = this->m_entities.view<cpnt::Transform, cpnt::AnimatedModel>();
        for ( const auto entity : viewAnimated ) {
            auto& cpntTrans = viewAnimated.get<cpnt::Transform>(entity);
            auto& cpntModel = viewAnimated.get<cpnt::AnimatedModel>(entity);

            uniloc.modelMat(cpntTrans.getMat());
            cpntModel.m_model->render(uniloc, cpntModel.m_animState.getTransformArray());
        }
    }

    void SceneGraph::render_staticOnEnvmap(const UniRender_Static& uniloc) {
        this->sendDlightUniform(uniloc.i_lighting);
        uniloc.i_envmap.hasEnvmap(false);

        for ( auto& map : this->m_mapChunks ) {
            map.m_map.render_staticOnEnvmap(uniloc);
        }

        const auto view = this->m_entities.view<cpnt::Transform, cpnt::StaticModel>();
        for ( const auto entity : view ) {
            auto& cpntTrans = view.get<cpnt::Transform>(entity);
            auto& cpntModel = view.get<cpnt::StaticModel>(entity);

            uniloc.modelMat(cpntTrans.getMat());
            cpntModel.m_model->render(uniloc);
        }
    }


    void SceneGraph::sendDlightUniform(const UniInterf_Lighting& uniloc) {
        uniloc.dlightCount(this->m_dlights.size());

        for ( size_t i = 0; i < this->m_dlights.size(); ++i ) {
            this->m_dlights[i].sendUniform(i, uniloc);
        }
    }


    std::optional<RayCastingResult> SceneGraph::doRayCasting(const Segment& ray) {
        std::optional<RayCastingResult> result{ std::nullopt };
        float closestDist = std::numeric_limits<float>::max();

        for ( auto& map : this->m_mapChunks ) {
            auto info = map.m_map.castRayToClosest(ray);
            if ( info ) {
                if ( info->m_distance < closestDist ) {
                    result = info;
                    closestDist = info->m_distance;
                }
            }
        }

        return result;
    }


    auto SceneGraph::findClosestEnv(const glm::vec3& pos) const -> const dal::EnvMap* {
        const dal::EnvMap* result = nullptr;
        float maxDist = std::numeric_limits<float>::max();

        for ( auto& map : this->m_mapChunks ) {
            if ( !map.m_info->m_aabb.isInside(pos) )
                continue;

            const auto [envmap, dist] = map.m_map.getClosestEnvMap(pos);
            if ( maxDist > dist ) {
                result = envmap;
                maxDist = dist;
            }
        }

        return result;
    }

    auto SceneGraph::findClosestMapChunk(const glm::vec3& pos) const -> const dal::MapChunk2* {
        for ( auto& map : this->m_mapChunks ) {
            if ( map.m_info->m_aabb.isInside(pos) )
                return &map.m_map;
        }

        return nullptr;
    }


    void SceneGraph::onResize(const unsigned int width, const unsigned int height) {
        for ( auto& map : m_mapChunks ) {
            map.m_map.onWinResize(width, height);
        }
    }

    // Private

    void SceneGraph::openLevel(const char* const respath) {
        std::vector<uint8_t> buffer;
        {
            auto file = dal::fileopen(respath, dal::FileMode2::bread);
            dalAssertm(file, fmt::format("failed to open file: {}", respath));
            buffer.resize(file->getSize());
            const auto readSize = file->read(buffer.data(), buffer.size());
            dalAssert(0 != readSize);
        }

        const auto map = dal::parseLevel_v1(buffer.data(), buffer.size());
        dalAssertm(map, fmt::format("failed to load level: {}", respath));

        this->m_activeLevel.setRespath(respath);
        this->m_activeLevel.clear();
        this->m_activeLevel.reserve(map->m_chunks.size());
        for ( const auto& chunkInfo : map->m_chunks ) {
            auto& chunk = this->m_activeLevel.newChunk();

            chunk.m_aabb.set(chunkInfo.m_aabb.m_min, chunkInfo.m_aabb.m_max);
            chunk.m_name = chunkInfo.m_name;
            chunk.m_offsetPos = chunkInfo.m_offsetPos;
        }

        this->m_dlights.clear();
        this->m_dlights.reserve(map->m_dlights.size());
        for ( const auto& dlightInfo : map->m_dlights ) {
            auto& dlight = this->m_dlights.emplace_back();

            dlight.m_color = dlightInfo.m_color * dlightInfo.m_intensity * 2.f;
            dlight.m_name = dlightInfo.m_name;
            dlight.setDirectin(dlightInfo.m_direction);
        }
    }

    void SceneGraph::openChunk(const char* const respath, const LevelData::ChunkData& info) {
        auto& map = this->m_mapChunks.emplace_back();
        map.m_map = this->m_resMas.loadChunk(respath);
        map.m_info = &info;
    }

}
