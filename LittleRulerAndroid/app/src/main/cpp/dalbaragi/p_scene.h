#pragma once

#include <string>
#include <vector>
#include <list>

#include "p_uniloc.h"
#include "u_loadinfo.h"
#include "p_resource.h"
#include "p_light.h"
#include "p_water.h"


namespace dal {

    class MapChunk {

    private:
        template <typename T>
        struct ModelNActor {
            T* m_model = nullptr;
            std::list<ActorInfo> m_inst;

            ModelNActor(void) = default;
            ModelNActor(T* const model) : m_model(model) {}
        };

    private:
        std::string m_name;

        std::vector<ModelNActor<ModelStatic>> m_modelActors;
        std::vector<ModelNActor<ModelAnimated>> m_animatedActors;

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
        void renderWaterry(const UnilocWaterry& uniloc);
        void renderAnimate(const UnilocAnimate& uniloc);
        void renderGeneral_onWater(const UnilocGeneral& uniloc, const ICamera& cam, MapChunk* const additional);
        void renderAnimate_onWater(const UnilocAnimate& uniloc, const ICamera& cam, MapChunk* const additional);

        int sendUniforms_lights(const UnilocGeneral& uniloc, int startIndex) const;
        int sendUniforms_lights(const UnilocWaterry& uniloc, int startIndex) const;

        void applyCollision(ModelStatic& model, ActorInfo& actor);

        WaterRenderer* getWater(const size_t index);
        ActorInfo* addActor(ModelStatic* const model, const std::string& actorName, bool flagStatic, ResourceMaster& resMas);
        ModelAnimated* getModelNActorAnimated(const ResourceID& resID);

    };


    class SceneMaster {

        //////// Attribs ////////

    private:
        ResourceMaster& m_resMas;
        std::list<MapChunk> m_mapChunks;
        MapChunk* m_persistantMap;

        //////// Methods ////////

    public:
        SceneMaster(ResourceMaster& resMas, const unsigned int winWidth, const unsigned int winHeight);
        ~SceneMaster(void);

        void update(const float deltaTime);

        void renderGeneral(const UnilocGeneral& uniloc);
        void renderDepthMp(const UnilocDepthmp& uniloc);
        void renderWaterry(const UnilocWaterry& uniloc);
        void renderAnimate(const UnilocAnimate& uniloc);
        void renderGeneral_onWater(const UnilocGeneral& uniloc, const ICamera& cam);
        void renderAnimate_onWater(const UnilocAnimate& uniloc, const ICamera& cam);

        ActorInfo* addActor(ModelStatic* const model, const std::string& mapName, const std::string& actorName, bool flagStatic);
        WaterRenderer* getWater(const std::string& mapName, const size_t index);
        ModelAnimated* getModelNActorAnimated(const ResourceID& resID, const std::string& mapName);

        void applyCollision(ModelStatic& model, ActorInfo& actor);

        void loadMap(const ResourceID& mapID);

        void onResize(const unsigned int width, const unsigned int height);

    private:
        void addMap(const loadedinfo::LoadedMap& map);
        MapChunk* findMap(const std::string& name);

    };

}