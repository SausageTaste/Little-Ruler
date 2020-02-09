#pragma once

#include <string>
#include <vector>
#include <list>

#include <entt/entity/registry.hpp>

#include "p_uniloc.h"
#include "u_loadinfo.h"
#include "p_resource.h"
#include "p_light.h"


namespace dal {

    /*
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
        MapChunk(const binfo::LoadedMap& info, ResourceMaster& resMan);

        const std::string& getName(void) const;

        void onScreanResize(const unsigned int width, const unsigned int height);
        void update(const float deltaTime);

        void renderGeneral(const UnilocGeneral& uniloc);
        void renderDepthMp(const UnilocDepthmp& uniloc);
        void renderDepthAnimated(const UnilocDepthAnime& uniloc);
        void renderWaterry(const UnilocWaterry& uniloc);
        void renderAnimate(const UnilocAnimate& uniloc);
        void renderOnWaterGeneral(const UnilocGeneral& uniloc, const ICamera& cam, entt::registry& reg);
        void renderOnWaterAnimated(const UnilocAnimate& uniloc, const ICamera& cam, entt::registry& reg);

        // Only for objects that are outside of this chunk.
        void applyCollision(const AABB& inOriginalBox, cpnt::Transform& inTrans);
        std::optional<RayCastingResult> doRayCasting(const Segment& ray);

        WaterRenderer* getWater(const size_t index);
        ActorInfo* addActor(ModelStaticHandle const model, const std::string& actorName, bool flagStatic, ResourceMaster& resMas);
        ModelAnimated* getModelNActorAnimated(const ResourceID& resID);

    private:
        int sendUniforms_lights(const UniInterfLightedMesh& uniloc, int startIndex) const;

    };
    */


    class SceneGraph {

        //////// Attribs ////////

    private:
        ResourceMaster& m_resMas;
        std::list<MapChunk2> m_mapChunks2;

    public:
        entt::registry m_entities;
        entt::entity m_player;
        StrangeEulerCamera m_playerCam;

        //////// Methods ////////

    public:
        SceneGraph(ResourceMaster& resMas, const unsigned int winWidth, const unsigned int winHeight);

        void update(const float deltaTime);

        void renderGeneral(const UnilocGeneral& uniloc);
        void renderAnimate(const UnilocAnimate& uniloc);
        void renderDepthGeneral(const UnilocDepthmp& uniloc);
        void renderDepthAnimated(const UnilocDepthAnime& uniloc);
        void renderWater(const UnilocWaterry& uniloc);

        void render_static(const UniRender_Static& uniloc);
        void render_animated(const UniRender_Animated& uniloc);
        void render_staticDepth(const UniRender_StaticDepth& uniloc);
        void render_animatedDepth(const UniRender_AnimatedDepth& uniloc);

        std::vector<WaterRenderer*> waters(void);

        void applyCollision(const ICollider& inCol, cpnt::Transform& inTrans);
        std::optional<RayCastingResult> doRayCasting(const Segment& ray);

        void onResize(const unsigned int width, const unsigned int height);

    };

}