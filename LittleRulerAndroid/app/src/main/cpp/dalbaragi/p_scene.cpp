#include "p_scene.h"

#include <unordered_set>
#include <cassert>

#include "s_logger_god.h"
#include "u_maploader.h"
#include "s_configs.h"


using namespace std::string_literals;


namespace {

    dal::StrangeEulerCamera makeReflectionCamera(const dal::ICamera& camera, const float waterHeight) {
        dal::StrangeEulerCamera newCam = *(reinterpret_cast<const dal::StrangeEulerCamera*>(&camera));

        newCam.m_pos.y = 2.0f * waterHeight - newCam.m_pos.y;

        const auto camViewPlane = newCam.getViewPlane();
        newCam.setViewPlane(camViewPlane.x, -camViewPlane.y);

        newCam.updateViewMat();
        return newCam;
    }

}


namespace dal {

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

            auto& [model, actors] = this->m_modelActors.emplace_back(resMas.orderModel(modelResID));
            actors.assign(importedModel.m_actors.begin(), importedModel.m_actors.end());
        }

        for ( auto& animatedModel : info.m_animatedModels ) {
            auto& [model, actors] = this->m_animatedActors.emplace_back();

            ResourceID modelResID{ animatedModel.m_modelID };
            if ( modelResID.getPackage().empty() ) modelResID.setPackage(info.m_packageName);

            model = resMas.orderModelAnimated(modelResID);
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

        const auto width = ConfigsGod::getinst().getWinWidth();
        const auto height = ConfigsGod::getinst().getWinHeight();
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
        for ( auto& model : this->m_animatedActors ) {
            model.first->updateAnimation0();
        }
    }


    void MapChunk::renderGeneral(const UnilocGeneral& uniloc) {
        this->sendUniforms_lights(uniloc.m_lightedMesh, 0);

        for ( auto& [model, actors] :this->m_modelActors ) {
            for ( auto& actor : actors ) {
                model.render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), actor.getModelMat());
            }
        }
    }

    void MapChunk::renderDepthMp(const UnilocDepthmp& uniloc) {
        for ( auto& [mdl, actors] : this->m_modelActors ) {
            for ( auto& actor : actors ) {
                mdl.renderDepthMap(uniloc.m_geometry, actor.getModelMat());
            }
        }
    }

    void MapChunk::renderDepthAnimated(const UnilocDepthAnime& uniloc) {
        for ( auto& modelActor : this->m_animatedActors ) {
            for ( auto& actor : modelActor.second ) {
                modelActor.first->renderDepthMap(uniloc.m_geometry, uniloc.m_anime, actor.getModelMat());
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

        for ( auto& modelActor : this->m_animatedActors ) {
            for ( auto& actor : modelActor.second ) {
                modelActor.first->render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), uniloc.m_anime, actor.getModelMat());
            }
        }
    }

    void MapChunk::renderGeneral_onWater(const UnilocGeneral& uniloc, const ICamera& cam, entt::registry& reg) {
        const auto view = reg.view<cpnt::Transform, cpnt::StaticModel>();

        for ( auto& water : this->m_waters ) {
            {
                // Uniform values

                uniloc.m_planeClip.flagDoClip(true);
                uniloc.m_planeClip.clipPlane(0.0f, 1.0f, 0.0f, -water.getHeight() + 0.1f);

                auto [reflectedPos, reflectedMat] = cam.makeReflected(water.getHeight());

                uniloc.m_lightedMesh.viewMat(reflectedMat);
                uniloc.m_lightedMesh.viewPos(reflectedPos);

                water.m_fbuffer.bindReflectionFrameBuffer();
                glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

                // Render map general
                this->renderGeneral(uniloc);

                view.each(
                    [&uniloc](const cpnt::Transform& trans, const cpnt::StaticModel& model) {
                        model.m_model->render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), trans.m_modelMat);
                    }
                );
            }

            {
                // Uniform values

                uniloc.m_planeClip.flagDoClip(true);
                uniloc.m_planeClip.clipPlane(0.0, -1.0, 0.0, water.getHeight());

                uniloc.m_lightedMesh.viewMat(cam.getViewMat());
                uniloc.m_lightedMesh.viewPos(cam.m_pos);

                water.m_fbuffer.bindRefractionFrameBuffer();
                glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

                // Render map general
                this->renderGeneral(uniloc);

                view.each(
                    [&uniloc](const cpnt::Transform& trans, const cpnt::StaticModel& model) {
                        model.m_model->render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), trans.m_modelMat);
                    }
                );
            }
        }
    }

    void MapChunk::renderAnimate_onWater(const UnilocAnimate& uniloc, const ICamera& cam, entt::registry& reg) {
        const auto view = reg.view<cpnt::Transform, cpnt::AnimatedModel>();

        for ( auto& water : this->m_waters ) {
            {
                // Uniform values

                uniloc.m_planeClip.flagDoClip(true);
                uniloc.m_planeClip.clipPlane(0.0f, 1.0f, 0.0f, -water.getHeight());

                auto [reflectedPos, reflectedMat] = cam.makeReflected(water.getHeight());

                uniloc.m_lightedMesh.viewMat(reflectedMat);
                uniloc.m_lightedMesh.viewPos(reflectedPos);

                water.m_fbuffer.bindReflectionFrameBuffer();
                //glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

                // Render map general
                this->renderAnimate(uniloc);

                view.each(
                    [&uniloc](const cpnt::Transform& trans, const cpnt::AnimatedModel& model) {
                        model.m_model->render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), uniloc.m_anime, trans.m_modelMat);
                    }
                );
            }

            {
                // Uniform values

                uniloc.m_planeClip.flagDoClip(true);
                uniloc.m_planeClip.clipPlane(0.0f, -1.0f, 0.0f, water.getHeight());

                uniloc.m_lightedMesh.viewMat(cam.getViewMat());
                uniloc.m_lightedMesh.viewPos(cam.m_pos);

                water.m_fbuffer.bindRefractionFrameBuffer();
                //glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

                // Render map general
                this->renderAnimate(uniloc);

                view.each(
                    [&uniloc](const cpnt::Transform& trans, const cpnt::AnimatedModel& model) {
                        model.m_model->render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), uniloc.m_anime, trans.m_modelMat);
                    }
                );
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

    void MapChunk::applyCollision(ModelStaticHandle& model, ActorInfo& actor) {
        auto inputActorBBox = model.getBoundingBox();
        inputActorBBox.add(actor.getPos());

        for ( auto& [mdl, actors] : this->m_modelActors ) {
            for ( auto& targetActor : actors ) {
                if ( &targetActor == &actor ) continue;

                auto targetBBox = model.getBoundingBox();
                targetBBox.add(targetActor.getPos());

                if ( checkCollision(inputActorBBox, targetBBox) ) {
                    const auto resolveInfo = calcResolveInfo(inputActorBBox, targetBBox);
                    actor.addPos(resolveInfo.m_this);
                    inputActorBBox.add(resolveInfo.m_this);
                    targetActor.addPos(resolveInfo.m_other);
                }
            }
        }

        for ( auto& [mdl, actors] : this->m_animatedActors ) {
            for ( auto& targetActor : actors ) {
                if ( &targetActor == &actor ) continue;

                auto targetBBox = model.getBoundingBox();
                targetBBox.add(targetActor.getPos());

                if ( checkCollision(inputActorBBox, targetBBox) ) {
                    const auto resolveInfo = calcResolveInfo(inputActorBBox, targetBBox);
                    actor.addPos(resolveInfo.m_this);
                    inputActorBBox.add(resolveInfo.m_this);
                    targetActor.addPos(resolveInfo.m_other);
                }
            }
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
                return &(actors.emplace_back(actorName, flagStatic));
            }
        }

        this->m_modelActors.emplace_back(resMas.orderModel(model.getResID()));
        auto& [mdl, actors] = this->m_modelActors.back();
        return &actors.emplace_back(actorName, flagStatic);
    }

    ModelAnimated* MapChunk::getModelNActorAnimated(const ResourceID& resID) {
        for ( auto& [mdl, actors] : this->m_animatedActors ) {
            const auto name = mdl->getModelResID().makeFileName();
            if ( name == resID.makeFileName() ) {
                return mdl;
            }
        }
        return nullptr;
    }

}


namespace dal {

    SceneMaster::SceneMaster(ResourceMaster& resMas, const unsigned int winWidth, const unsigned int winHeight)
        : m_resMas(resMas)
    {
        // This is needed by Water objects
        {
            ConfigsGod::getinst().setWinSize(winWidth, winHeight);
        }

        this->loadMap("asset::map/water_bowl.dlb");
    }

    SceneMaster::~SceneMaster(void) {

    }


    void SceneMaster::update(const float deltaTime) {
        for ( auto& map : this->m_mapChunks ) {
            map.update(deltaTime);
        }
    }

    void SceneMaster::renderGeneral(const UnilocGeneral& uniloc) {
        for ( auto& map : m_mapChunks ) {
            map.renderGeneral(uniloc);
        }
    }

    void SceneMaster::renderDepthMp(const UnilocDepthmp& uniloc) {
        for ( auto& map : m_mapChunks ) {
            map.renderDepthMp(uniloc);
        }
    }

    void SceneMaster::renderDepthAnimated(const UnilocDepthAnime& uniloc) {
        for ( auto& map : m_mapChunks ) {
            map.renderDepthAnimated(uniloc);
        }
    }

    void SceneMaster::renderWaterry(const UnilocWaterry& uniloc) {
        for ( auto& map : m_mapChunks ) {
            map.renderWaterry(uniloc);
        }
    }

    void SceneMaster::renderAnimate(const UnilocAnimate& uniloc) {
        for ( auto& map : m_mapChunks ) {
            map.renderAnimate(uniloc);
        }
    }

    void SceneMaster::renderGeneral_onWater(const UnilocGeneral& uniloc, const ICamera& cam, entt::registry& reg) {
        for ( auto& map : this->m_mapChunks ) {
            map.renderGeneral_onWater(uniloc, cam, reg);
        }
    }

    void SceneMaster::renderAnimate_onWater(const UnilocAnimate& uniloc, const ICamera& cam, entt::registry& reg) {
        for (auto& map : this->m_mapChunks ) {
            map.renderAnimate_onWater(uniloc, cam, reg);
        }
    }


    ActorInfo* SceneMaster::addActor(ModelStaticHandle model, const std::string& mapName, const std::string& actorName, bool flagStatic) {
        auto map = this->findMap(mapName);
        if ( nullptr == map ) {
            return nullptr;
        }
        return map->addActor(model, actorName, flagStatic, this->m_resMas);
    }

    WaterRenderer* SceneMaster::getWater(const std::string& mapName, const size_t index) {
        for ( auto& map : this->m_mapChunks ) {
            if ( mapName == map.getName() ) {
                return map.getWater(index);
            }
        }

        return nullptr;
    }

    ModelAnimated* SceneMaster::getModelNActorAnimated(const ResourceID& resID, const std::string& mapName) {
        for ( auto& map : this->m_mapChunks ) {
            if ( mapName == map.getName() ) {
                return map.getModelNActorAnimated(resID);
            }
        }
        return nullptr;
    }

    void SceneMaster::applyCollision(ModelStaticHandle& model, ActorInfo& actor) {
        for ( auto& map : this->m_mapChunks ) {
            map.applyCollision(model, actor);
        }
    }


    void SceneMaster::loadMap(const ResourceID& mapID) {
        std::vector<uint8_t> buffer;
        auto res = futil::getRes_buffer(mapID, buffer);
        if ( !res ) {
            dalError("Failed to load map file: "s + mapID.makeIDStr());
            return;
        }

        loadedinfo::LoadedMap info;
        info.m_mapName = mapID.getBareName();
        info.m_packageName = mapID.getPackage();

        res = parseMap_dlb(info, buffer.data(), buffer.size());
        if ( !res ) {
            dalError("Failed to parse level: "s + mapID.makeIDStr());
            return;
        }

        this->addMap(info);
    }

    void SceneMaster::onResize(const unsigned int width, const unsigned int height) {
        for ( auto& map : this->m_mapChunks ) {
            map.onScreanResize(width, height);
        }
    }

    // Private

    void SceneMaster::addMap(const loadedinfo::LoadedMap& map) {
        this->m_mapChunks.emplace_back(map, this->m_resMas);
        dalInfo("Map added: "s + this->m_mapChunks.back().getName());
    }

    MapChunk* SceneMaster::findMap(const std::string& name) {
        if ( name.empty() ) {
            dalError("Failed to find map because map name was not given.");
            return nullptr;
        }

        for ( auto& map : this->m_mapChunks ) {
            if ( name == map.getName() ) {
                return &map;
            }
        }

        dalError("Failed to find map in SceneMaster: "s + name);
        return nullptr;
    }

}