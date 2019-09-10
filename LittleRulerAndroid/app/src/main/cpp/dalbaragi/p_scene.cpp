#include "p_scene.h"

#include <unordered_set>
#include <cassert>

#include <fmt/format.h>

#include "s_logger_god.h"
#include "u_maploader.h"
#include "s_configs.h"


using namespace fmt::literals;


namespace dal {

    /*
    MapChunk::MapChunk(const std::string& name)
        : m_name(name)
    {

    }

    MapChunk::MapChunk(const loadedinfo::LoadedMap& info, ResourceMaster& resMas)
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

    void MapChunk::renderGeneral_onWater(const UnilocGeneral& uniloc, const ICamera& cam, entt::registry& reg) {
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

    void MapChunk::renderAnimate_onWater(const UnilocAnimate& uniloc, const ICamera& cam, entt::registry& reg) {
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

    std::optional<RayCastingResult> MapChunk::doRayCasting(const Ray& ray) {
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
    }


    void SceneGraph::update(const float deltaTime) {

    }

    void SceneGraph::renderGeneral(const UnilocGeneral& uniloc) {
        for ( auto& map : this->m_mapChunks2 ) {
            map.renderGeneral(uniloc);
        }
    }

    void SceneGraph::renderDepthMp(const UnilocDepthmp& uniloc) {
        for ( auto& map : this->m_mapChunks2 ) {
            map.renderDepthMp(uniloc);
        }
    }

    void SceneGraph::renderDepthAnimated(const UnilocDepthAnime& uniloc) {

    }

    void SceneGraph::renderWaterry(const UnilocWaterry& uniloc) {
        for ( auto& map : m_mapChunks2 ) {
            map.renderWater(uniloc);
        }
    }

    void SceneGraph::renderAnimate(const UnilocAnimate& uniloc) {

    }

    void SceneGraph::renderGeneral_onWater(const UnilocGeneral& uniloc, const ICamera& cam, entt::registry& reg) {
        for ( auto& map : this->m_mapChunks2 ) {
            map.renderOnWaterGeneral(uniloc, cam, reg);
        }
    }

    void SceneGraph::renderAnimate_onWater(const UnilocAnimate& uniloc, const ICamera& cam, entt::registry& reg) {
        for ( auto& map : this->m_mapChunks2 ) {
            map.renderOnWaterAnimated(uniloc, cam, reg);
        }
    }

    void SceneGraph::applyCollision(const AABB& inOriginalBox, cpnt::Transform& inTrans) {
        const ColAABB absAABB{ inOriginalBox };
        for ( auto& map : this->m_mapChunks2 ) {
            map.applyCollision(absAABB, inTrans);
        }
    }

    std::optional<RayCastingResult> SceneGraph::doRayCasting(const Ray& ray) {
        RayCastingResult result;
        bool found = false;

        for ( auto& map : this->m_mapChunks2 ) {
            auto info = map.castRayToClosest(ray);

            if ( info ) {
                if ( found ) {
                    if ( info->m_distance < result.m_distance ) {
                        result = *info;
                    }
                }
                else {
                    result = *info;
                    found = true;
                }
            }
        }

        if ( found ) {
            return result;
        }
        else {
            return std::nullopt;
        }
    }

    void SceneGraph::onResize(const unsigned int width, const unsigned int height) {

    }

}