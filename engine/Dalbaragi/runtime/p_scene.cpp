#include "p_scene.h"

#include <fmt/format.h>

#include <d_logger.h>

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


namespace dal {

    SceneGraph::SceneGraph(ResourceMaster& resMas, const unsigned int winWidth, const unsigned int winHeight)
        : m_resMas(resMas)
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
    }


    void SceneGraph::update(const float deltaTime) {
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
