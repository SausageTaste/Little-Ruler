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

    /*
    MapChunk::MapChunk(const std::string& name)
        : m_name(name)
    {

    }

    MapChunk::MapChunk(const binfo::LoadedMap& info, ResourceMaster& resMas)
        : m_name(info.m_mapName)
    {
        for ( auto& definedModel : info.m_definedModels ) {
            auto& [model, actors] = this->m_modelActors.emplace_back(resMas.buildModel(definedModel, info.m_packageName.c_str()));
            actors.assign(definedModel.m_actors.begin(), definedModel.m_actors.end());
        }

        for ( auto& importedModel : info.m_importedModels ) {
            ResourceID modelResID{ importedModel.m_modelID };
            if ( modelResID.getPackage().empty() ) {
                modelResID.setPackage(info.m_packageName);
            }

            auto modelHandle = resMas.orderModelStatic(modelResID);
            auto& [model, actors] = this->m_modelActors.emplace_back(std::move(modelHandle));
            actors.assign(importedModel.m_actors.begin(), importedModel.m_actors.end());
        }

        for ( auto& animatedModel : info.m_animatedModels ) {
            auto& [model, actors, animStat] = this->m_animatedActors.emplace_back();

            ResourceID modelResID{ animatedModel.m_modelID };
            if ( modelResID.getPackage().empty() ) modelResID.setPackage(info.m_packageName);

            model = resMas.orderModelAnim(modelResID);
            actors.assign(animatedModel.m_actors.begin(), animatedModel.m_actors.end());
        }

        for ( auto& pointLight : info.m_pointLights ) {
            this->m_plights.emplace_back();
            auto& plight = this->m_plights.back();

            plight.m_name = pointLight.m_name;
            plight.m_color = pointLight.m_color;
            plight.mPos = pointLight.m_pos;
            plight.mMaxDistance = pointLight.m_maxDist;
        }

        const auto width = GlobalStateGod::getinst().getWinWidth();
        const auto height = GlobalStateGod::getinst().getWinHeight();
        for ( auto& waterInfo : info.m_waterPlanes ) {
            this->m_waters.emplace_back(waterInfo, width, height);
        }
    }

    const std::string& MapChunk::getName(void) const {
        return this->m_name;
    }

    void MapChunk::onScreanResize(const unsigned int width, const unsigned int height) {
        for ( auto& water : this->m_waters ) {
            water.m_fbuffer.resizeFbuffer(width, height);
        }
    }

    void MapChunk::update(const float deltaTime) {
        for ( auto& [mdl, actors, animStat] : this->m_animatedActors ) {
            auto& anims = mdl->getAnimations();
            if ( anims.empty() ) {
                continue;
            }

            const auto& anim = anims.back();
            const auto elapsed = animStat.getElapsed();
            const auto animTick = anim.calcAnimTick(elapsed);
            anim.sample(animTick, mdl->getSkeletonInterf(), mdl->getGlobalInvMat(), animStat.getTransformArray());
        }
    }


    void MapChunk::renderGeneral(const UnilocGeneral& uniloc) {
        this->sendUniforms_lights(uniloc.m_lightedMesh, 0);

        for ( const auto& [model, actors] :this->m_modelActors ) {
            for ( const auto& actor : actors ) {
                model.render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), actor.m_transform.getMat());
            }
        }
    }

    void MapChunk::renderDepthMp(const UnilocDepthmp& uniloc) {
        for ( const auto& [mdl, actors] : this->m_modelActors ) {
            for ( const auto& actor : actors ) {
                mdl.renderDepthMap(uniloc.m_geometry, actor.m_transform.getMat());
            }
        }
    }

    void MapChunk::renderDepthAnimated(const UnilocDepthAnime& uniloc) {
        for ( auto& [mdl, actors, animStat] : this->m_animatedActors ) {
            for ( auto& actor : actors ) {
                mdl->renderDepthMap(uniloc.m_geometry, uniloc.m_anime, actor.m_transform.getMat(), animStat.getTransformArray());
            }
        }
    }

    void MapChunk::renderWaterry(const UnilocWaterry& uniloc) {
        this->sendUniforms_lights(uniloc.m_lightedMesh, 0);

        for ( auto& water : this->m_waters ) {
            water.renderWaterry(uniloc);
        }
    }

    void MapChunk::renderAnimate(const UnilocAnimate& uniloc) {
        this->sendUniforms_lights(uniloc.m_lightedMesh, 0);

        for ( auto& [mdl, actors, animStat] : this->m_animatedActors ) {
            for ( auto& actor : actors ) {
                mdl->render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), uniloc.m_anime, actor.m_transform.getMat(), animStat.getTransformArray());
            }
        }
    }

    void MapChunk::renderOnWaterGeneral(const UnilocGeneral& uniloc, const ICamera& cam, entt::registry& reg) {
        const auto view = reg.view<cpnt::Transform, cpnt::StaticModel>();

        for ( auto& water : this->m_waters ) {
            {
                water.startRenderOnReflec(uniloc, cam);
                glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
                this->renderGeneral(uniloc);

                view.each(
                    [&uniloc](const cpnt::Transform& trans, const cpnt::StaticModel& model) {
                        model.m_model.render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), trans.getMat());
                    }
                );
            }

            {
                water.startRenderOnRefrac(uniloc, cam);
                glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
                this->renderGeneral(uniloc);

                view.each(
                    [&uniloc](const cpnt::Transform& trans, const cpnt::StaticModel& model) {
                        model.m_model.render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), trans.getMat());
                    }
                );
            }
        }
    }

    void MapChunk::renderOnWaterAnimated(const UnilocAnimate& uniloc, const ICamera& cam, entt::registry& reg) {
        const auto view = reg.view<cpnt::Transform, cpnt::AnimatedModel>();

        for ( auto& water : this->m_waters ) {
            {
                water.startRenderOnReflec(uniloc, cam);
                this->renderAnimate(uniloc);

                for ( const auto entity : view ) {
                    auto& cpntTransform = view.get<cpnt::Transform>(entity);
                    auto& cpntModel = view.get<cpnt::AnimatedModel>(entity);

                    cpntModel.m_model.render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), uniloc.m_anime, cpntTransform.getMat(),
                        cpntModel.m_animState.getTransformArray());
                }
            }

            {
                water.startRenderOnRefrac(uniloc, cam);
                this->renderAnimate(uniloc);

                for ( const auto entity : view ) {
                    auto& cpntTransform = view.get<cpnt::Transform>(entity);
                    auto& cpntModel = view.get<cpnt::AnimatedModel>(entity);

                    cpntModel.m_model.render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), uniloc.m_anime, cpntTransform.getMat(),
                        cpntModel.m_animState.getTransformArray());
                }
            }
        }
    }


    int MapChunk::sendUniforms_lights(const UniInterfLightedMesh& uniloc, int startIndex) const {
        if ( startIndex >= 3 ) dalAbort("Too many point lights.");
        if ( startIndex + this->m_plights.size() > 3 ) dalAbort("Too many point lights.");

        uniloc.plightCount(startIndex + this->m_plights.size());
        for ( size_t i = 0; i < this->m_plights.size(); i++ ) {
            if ( i >= 3 ) break;
            this->m_plights.at(i).sendUniform(uniloc, startIndex + i);
        }

        return startIndex + this->m_plights.size();
    }

    void MapChunk::applyCollision(const AABB& inOriginalBox, cpnt::Transform& inTrans) {
        for ( auto& [mdl, actors] : this->m_modelActors ) {
            for ( auto& targetActor : actors ) {
                if ( checkCollision(inOriginalBox, mdl.getBounding(), inTrans, targetActor.m_transform) ) {
                    PhysicalProperty massInv1; massInv1.setMassInv(1.f);
                    const auto resolveInfo = calcResolveInfo(
                        inOriginalBox, mdl.getBoundingBox(), massInv1, PhysicalProperty{}, inTrans, targetActor.m_transform
                    );
                    inTrans.addPos(resolveInfo.m_this);
                    targetActor.m_transform.addPos(resolveInfo.m_other);
                }
            }
        }

        for ( auto& [mdl, actors, animStat] : this->m_animatedActors ) {
            for ( auto& targetActor : actors ) {
                if ( checkCollision(inOriginalBox, mdl->getBoundingBox(), inTrans, targetActor.m_transform) ) {
                    PhysicalProperty massInv1; massInv1.setMassInv(1.f);
                    const auto resolveInfo = calcResolveInfo(
                        inOriginalBox, mdl->getBoundingBox(), massInv1, PhysicalProperty{}, inTrans, targetActor.m_transform
                    );
                    inTrans.addPos(resolveInfo.m_this);
                    targetActor.m_transform.addPos(resolveInfo.m_other);
                }
            }
        }
    }

    std::optional<RayCastingResult> MapChunk::doRayCasting(const Segment& ray) {
        RayCastingResult result;
        bool found = false;

        auto innerFunc = [&](const AABB& aabb, const Transform& aabbTrans) -> void {
            const auto info = calcCollisionInfo(ray, aabb, aabbTrans);
            if ( info ) {
                if ( found ) {
                    if ( info->m_distance < result.m_distance ) {
                        result = *info;
                        found = true;
                    }
                }
                else {
                    result = *info;
                    found = true;
                }
            }
        };

        for ( auto& [mdl, actors] : this->m_modelActors ) {
            for ( auto& targetActor : actors ) {
                innerFunc(mdl.getBoundingBox(), targetActor.m_transform);
            }
        }

        for ( auto& [mdl, actors, animStat] : this->m_animatedActors ) {
            for ( auto& targetActor : actors ) {
                innerFunc(mdl->getBoundingBox(), targetActor.m_transform);
            }
        }

        if ( found ) {
            return result;
        }
        else {
            return std::nullopt;
        }
    }


    WaterRenderer* MapChunk::getWater(const size_t index) {
        if ( index >= this->m_waters.size() ) {
            return nullptr;
        }
        else {
            return &this->m_waters.at(index);
        }
    }

    ActorInfo* MapChunk::addActor(ModelStaticHandle const model, const std::string& actorName, bool flagStatic, ResourceMaster& resMas) {
        for ( auto& [mdl, actors] : this->m_modelActors ) {
            if ( model == mdl ) {
                return &(actors.emplace_back(actorName));
            }
        }

        this->m_modelActors.emplace_back(resMas.orderModelStatic(model.getResID()));
        auto& [mdl, actors] = this->m_modelActors.back();
        return &actors.emplace_back(actorName);
    }

    ModelAnimated* MapChunk::getModelNActorAnimated(const ResourceID& resID) {
        for ( auto& [mdl, actors, animStat] : this->m_animatedActors ) {
            const auto name = mdl->m_resID.makeFileName();
            if ( name == resID.makeFileName() ) {
                return mdl;
            }
        }
        return nullptr;
    }
    */

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


    void SceneGraph::renderGeneral(const UnilocGeneral& uniloc) {
        for ( auto& map : this->m_mapChunks2 ) {
            map.renderGeneral(uniloc);
        }

        this->m_entities.view<cpnt::Transform, cpnt::StaticModel>().each(
            [&uniloc](const cpnt::Transform& trans, const cpnt::StaticModel& model) {
                model.m_model->render(uniloc.m_lightedMesh, uniloc.m_lightmaps, trans.getMat());
            }
        );
    }

    void SceneGraph::renderAnimate(const UnilocAnimate& uniloc) {
        const auto viewAnimated = this->m_entities.view<cpnt::Transform, cpnt::AnimatedModel>();
        for ( const auto entity : viewAnimated ) {
            auto& cpntTrans = viewAnimated.get<cpnt::Transform>(entity);
            auto& cpntModel = viewAnimated.get<cpnt::AnimatedModel>(entity);

            cpntModel.m_model->render(uniloc.m_lightedMesh, uniloc.m_lightmaps, uniloc.m_anime, cpntTrans.getMat(),
                cpntModel.m_animState.getTransformArray());
        }
    }

    void SceneGraph::renderDepthGeneral(const UnilocDepthmp& uniloc) {
        for ( auto& map : this->m_mapChunks2 ) {
            map.renderDepthGeneral(uniloc);
        }

        this->m_entities.view<cpnt::Transform, cpnt::StaticModel>().each(
            [&uniloc](const cpnt::Transform& trans, const cpnt::StaticModel& model) {
                model.m_model->renderDepth(uniloc.m_geometry, trans.getMat());
            }
        );
    }

    void SceneGraph::renderDepthAnimated(const UnilocDepthAnime& uniloc) {
        const auto viewAnimated = this->m_entities.view<cpnt::Transform, cpnt::AnimatedModel>();
        for ( const auto entity : viewAnimated ) {
            auto& cpntTrans = viewAnimated.get<cpnt::Transform>(entity);
            auto& cpntModel = viewAnimated.get<cpnt::AnimatedModel>(entity);

            cpntModel.m_model->renderDepth(uniloc.m_geometry, uniloc.m_anime, cpntTrans.getMat(), cpntModel.m_animState.getTransformArray());
        }
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


    void SceneGraph::renderOnWaterGeneral(const UnilocGeneral& uniloc, const ICamera& cam, entt::registry& reg) {
        for ( auto& map : this->m_mapChunks2 ) {
            map.renderOnWaterGeneral(uniloc, cam, reg);
        }
    }

    void SceneGraph::renderOnWaterAnimated(const UnilocAnimate& uniloc, const ICamera& cam, entt::registry& reg) {
        for ( auto& map : this->m_mapChunks2 ) {
            map.renderOnWaterAnimated(uniloc, cam, reg);
        }
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
