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
            this->m_modelActors.emplace_back();
            auto& modelActor = this->m_modelActors.back();

            modelActor.m_model = resMas.buildModel(definedModel, info.m_packageName.c_str());
            modelActor.m_inst.assign(definedModel.m_actors.begin(), definedModel.m_actors.end());
        }

        for ( auto& importedModel : info.m_importedModels ) {
            this->m_modelActors.emplace_back();
            auto& model = this->m_modelActors.back();

            ResourceID modelResID{ importedModel.m_modelID };
            if ( modelResID.getPackage().empty() ) modelResID.setPackage(info.m_packageName);

            model.m_model = resMas.orderModel(modelResID);
            model.m_inst.assign(importedModel.m_actors.begin(), importedModel.m_actors.end());
        }

        for ( auto& animatedModel : info.m_animatedModels ) {
            this->m_animatedActors.emplace_back();
            auto& model = this->m_animatedActors.back();

            ResourceID modelResID{ animatedModel.m_modelID };
            if ( modelResID.getPackage().empty() ) modelResID.setPackage(info.m_packageName);

            model.m_model = resMas.orderModelAnimated(modelResID);
            model.m_inst.assign(animatedModel.m_actors.begin(), animatedModel.m_actors.end());
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
            model.m_model->updateAnimation0();
        }
    }


    void MapChunk::renderGeneral(const UnilocGeneral& uniloc) {
        this->sendUniforms_lights(uniloc.m_lightedMesh, 0);

        for ( auto& modelActor : this->m_modelActors ) {
            modelActor.m_model->render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), modelActor.m_inst);
        }
    }

    void MapChunk::renderDepthMp(const UnilocDepthmp& uniloc) {
        for ( auto& modelActor : this->m_modelActors ) {
            modelActor.m_model->renderDepthMap(uniloc.m_geometry, modelActor.m_inst);
        }
    }

    void MapChunk::renderDepthAnimated(const UnilocDepthAnime& uniloc) {
        for ( auto& model : this->m_animatedActors ) {
            model.m_model->renderDepthMap(uniloc.m_geometry, uniloc.m_anime, model.m_inst);
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
            modelActor.m_model->render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), uniloc.m_anime, modelActor.m_inst);
        }
    }

    void MapChunk::renderGeneral_onWater(const UnilocGeneral& uniloc, const ICamera& cam, MapChunk* const additional) {
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

                if ( nullptr != additional ) additional->renderGeneral(uniloc);
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
                if ( nullptr != additional ) additional->renderGeneral(uniloc);
            }
        }
    }

    void MapChunk::renderAnimate_onWater(const UnilocAnimate& uniloc, const ICamera& cam, MapChunk* const additional) {
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

                if ( nullptr != additional ) additional->renderGeneral(uniloc);
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
                if ( nullptr != additional ) additional->renderAnimate(uniloc);
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

    void MapChunk::applyCollision(ModelStatic& model, ActorInfo& actor) {
        auto actorBox = model.getBoundingBox();
        actorBox.add(actor.getPos());

        for ( auto& modelInfo : this->m_modelActors ) {
            for ( auto& modelActor : modelInfo.m_inst ) {
                if ( &modelActor == &actor ) continue;

                auto box = modelInfo.m_model->getBoundingBox();
                box.add(modelActor.getPos());

                if ( checkCollision(actorBox, box) ) {
                    const auto resolveInfo = calcResolveInfo(actorBox, box);
                    actor.addPos(resolveInfo.m_this);
                    actorBox.add(resolveInfo.m_this);
                    modelActor.addPos(resolveInfo.m_other);
                }
            }
        }

        for ( auto& modelInfo : this->m_animatedActors ) {
            for ( auto& modelActor : modelInfo.m_inst ) {
                if ( &modelActor == &actor ) continue;

                auto box = modelInfo.m_model->getBoundingBox();
                box.add(modelActor.getPos());

                if ( checkCollision(actorBox, box) ) {
                    const auto resolveInfo = calcResolveInfo(actorBox, box);
                    actor.addPos(resolveInfo.m_this);
                    actorBox.add(resolveInfo.m_this);
                    modelActor.addPos(resolveInfo.m_other);
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

    ActorInfo* MapChunk::addActor(ModelStatic* const model, const std::string& actorName, bool flagStatic, ResourceMaster& resMas) {
        for ( auto& modelActor : this->m_modelActors ) {
            if ( model == modelActor.m_model ) {
                modelActor.m_inst.emplace_back(actorName, flagStatic);
                return &modelActor.m_inst.back();
            }
        }

        auto takenModel = resMas.orderModel(model->getModelResID());
        if ( nullptr == takenModel ) dalAbort("WTF??");
        this->m_modelActors.emplace_back(takenModel);
        auto& modelActor = this->m_modelActors.back();
        modelActor.m_inst.emplace_back(actorName, flagStatic);
        return &modelActor.m_inst.back();
    }

    ModelAnimated* MapChunk::getModelNActorAnimated(const ResourceID& resID) {
        for ( auto& x : this->m_animatedActors ) {
            const auto name = x.m_model->getModelResID().makeFileName();
            if ( name == resID.makeFileName() ) {
                return x.m_model;
            }
        }
        return nullptr;
    }

}


namespace dal {

    SceneMaster::SceneMaster(ResourceMaster& resMas, const unsigned int winWidth, const unsigned int winHeight)
        : m_resMas(resMas)
    {
        m_mapChunks.emplace_front("persis");
        m_persistantMap = &m_mapChunks.front();

        // This is needed by Water objects
        {
            ConfigsGod::getinst().setWinSize(winWidth, winHeight);
        }

        this->loadMap("asset::map/test_level.dlb");
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

    void SceneMaster::renderGeneral_onWater(const UnilocGeneral& uniloc, const ICamera& cam) {
        auto iter = this->m_mapChunks.begin();
        const auto end = this->m_mapChunks.end();
        iter->renderGeneral_onWater(uniloc, cam, nullptr);
        ++iter;

        while ( end != iter ) {
            iter->renderGeneral_onWater(uniloc, cam, this->m_persistantMap);
            ++iter;
        }
    }

    void SceneMaster::renderAnimate_onWater(const UnilocAnimate& uniloc, const ICamera& cam) {
        auto iter = this->m_mapChunks.begin();
        const auto end = this->m_mapChunks.end();
        iter->renderAnimate_onWater(uniloc, cam, nullptr);
        ++iter;

        while ( end != iter ) {
            iter->renderAnimate_onWater(uniloc, cam, this->m_persistantMap);
            ++iter;
        }
    }


    ActorInfo* SceneMaster::addActor(ModelStatic* const model, const std::string& mapName, const std::string& actorName, bool flagStatic) {
        auto map = mapName.empty() ? this->m_persistantMap : this->findMap(mapName);
        if ( nullptr == map ) {
            dalError("Failed to find map: "s + mapName);
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

    void SceneMaster::applyCollision(ModelStatic& model, ActorInfo& actor) {
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
        for ( auto& map : this->m_mapChunks ) {
            if ( name == map.getName() ) {
                return &map;
            }
        }

        dalWarn("Failed to find map in SceneMaster: "s + name);
        return nullptr;
    }

}