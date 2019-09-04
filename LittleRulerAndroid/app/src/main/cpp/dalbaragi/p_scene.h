#pragma once

#include <string>
#include <vector>
#include <list>

#include <entt/entity/registry.hpp>

#include "p_uniloc.h"
#include "u_loadinfo.h"
#include "p_resource.h"
#include "p_light.h"
#include "p_water.h"


namespace dal {

    class MapChunk {

    private:
        struct StaticModelActor {
            ModelStaticHandle m_model;
            std::list<ActorInfo> m_actors;

            StaticModelActor(ModelStaticHandle&& model) : m_model(std::move(model)) {}
        };

    private:
        std::string m_name;

        std::vector<StaticModelActor> m_modelActors;
        std::vector<std::tuple<ModelAnimated*, std::list<ActorInfo>, AnimationState>> m_animatedActors;

        std::vector<DirectionalLight> m_dlights;
        std::vector<PointLight> m_plights;

        std::vector<WaterRenderer> m_waters;

    public:
        MapChunk(const std::string& name);
        MapChunk(const loadedinfo::LoadedMap& info, ResourceMaster& resMan);

        const std::string& getName(void) const;

        void onScreanResize(const unsigned int width, const unsigned int height);
        void update(const float deltaTime);

        void renderGeneral(const UnilocGeneral& uniloc);
        void renderDepthMp(const UnilocDepthmp& uniloc);
        void renderDepthAnimated(const UnilocDepthAnime& uniloc);
        void renderWaterry(const UnilocWaterry& uniloc);
        void renderAnimate(const UnilocAnimate& uniloc);
        void renderGeneral_onWater(const UnilocGeneral& uniloc, const ICamera& cam, entt::registry& reg);
        void renderAnimate_onWater(const UnilocAnimate& uniloc, const ICamera& cam, entt::registry& reg);

        // Only for objects that are outside of this chunk.
        void applyCollision(const AABB& inOriginalBox, cpnt::Transform& inTrans);
        std::optional<RayCastingResult> doRayCasting(const Ray& ray);

        WaterRenderer* getWater(const size_t index);
        ActorInfo* addActor(ModelStaticHandle const model, const std::string& actorName, bool flagStatic, ResourceMaster& resMas);
        ModelAnimated* getModelNActorAnimated(const ResourceID& resID);

    private:
        int sendUniforms_lights(const UniInterfLightedMesh& uniloc, int startIndex) const;

    };


    class SceneGraph {

        //////// Attribs ////////

    private:
        ResourceMaster& m_resMas;
        std::list<MapChunk> m_mapChunks;
        std::list<MapChunk2> m_mapChunks2;

        //////// Methods ////////

    public:
        SceneGraph(ResourceMaster& resMas, const unsigned int winWidth, const unsigned int winHeight);
        ~SceneGraph(void);

        void update(const float deltaTime);

        void renderGeneral(const UnilocGeneral& uniloc);
        void renderDepthMp(const UnilocDepthmp& uniloc);
        void renderDepthAnimated(const UnilocDepthAnime& uniloc);
        void renderWaterry(const UnilocWaterry& uniloc);
        void renderAnimate(const UnilocAnimate& uniloc);
        void renderGeneral_onWater(const UnilocGeneral& uniloc, const ICamera& cam, entt::registry& reg);
        void renderAnimate_onWater(const UnilocAnimate& uniloc, const ICamera& cam, entt::registry& reg);

        void applyCollision(const AABB& inOriginalBox, cpnt::Transform& inTrans);
        std::optional<RayCastingResult> doRayCasting(const Ray& ray);

        void loadMap(const ResourceID& mapID);

        void onResize(const unsigned int width, const unsigned int height);

    private:
        void addMap(const loadedinfo::LoadedMap& map);
        MapChunk* findMap(const std::string& name);

    };

}