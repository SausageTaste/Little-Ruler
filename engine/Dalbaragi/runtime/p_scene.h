#pragma once

#include <string>
#include <vector>
#include <list>

#include <entt/entity/registry.hpp>

#include <d_phyworld.h>

#include "p_uniloc.h"
#include "u_loadinfo.h"
#include "p_resource.h"
#include "p_light.h"


namespace dal {

    class IEntityController {

    public:
        virtual ~IEntityController(void) = default;
        virtual void apply(const entt::entity entity, entt::registry& reg) = 0;

    };

    namespace cpnt {

        struct EntityCtrl {
            std::shared_ptr<IEntityController> m_ctrler;
        };

    }

}


namespace dal {

    class LevelData {

    public:
        struct ChunkData {
            std::string m_name;
            AABB m_aabb;
            glm::vec3 m_offsetPos{ 0 };
            bool m_active = false;
        };

    private:
        std::vector<ChunkData> m_chunks;

        std::string m_respath;

    public:
        ChunkData& at(const size_t index) {
            return this->m_chunks.at(index);
        }
        ChunkData& newChunk(void);
        void clear(void);
        size_t size(void) const;
        void reserve(const size_t s);


        const std::string& respath(void) const {
            return this->m_respath;
        }
        void setRespath(const std::string& respath);

    };


    class SceneGraph {

    private:
        struct MapChunkPack {
            MapChunk2 m_map;
            const LevelData::ChunkData* m_info = nullptr;
        };

        //////// Attribs ////////

    private:
        ResourceMaster& m_resMas;
        PhysicsWorld& m_phyworld;

    public:
        LevelData m_activeLevel;
        std::list<MapChunkPack> m_mapChunks;
        std::vector<DirectionalLight> m_dlights;

        entt::registry m_entities;
        entt::entity m_player;
        StrangeEulerCamera m_playerCam;

        //////// Methods ////////

    public:
        SceneGraph(ResourceMaster& resMas, PhysicsWorld& phyworld, const unsigned int winWidth, const unsigned int winHeight);

        void update(const float deltaTime);

        entt::entity addObj_static(const char* const resid);

        void render_static(const UniRender_Static& uniloc);
        void render_animated(const UniRender_Animated& uniloc);
        void render_staticDepth(const UniRender_StaticDepth& uniloc);
        void render_animatedDepth(const UniRender_AnimatedDepth& uniloc);
        void render_staticOnWater(const UniRender_StaticOnWater& uniloc);
        void render_animatedOnWater(const UniRender_AnimatedOnWater& uniloc);
        void render_staticOnEnvmap(const UniRender_Static& uniloc);

        void sendDlightUniform(const UniInterf_Lighting& uniloc);

        void applyCollision(const ICollider& inCol, cpnt::Transform& inTrans);
        std::optional<RayCastingResult> doRayCasting(const Segment& ray);

        auto findClosestEnv(const glm::vec3& pos) const -> const dal::EnvMap*;

        void onResize(const unsigned int width, const unsigned int height);

    private:
        void openLevel(const char* const respath);
        void openChunk(const char* const respath, const LevelData::ChunkData& info);

    };

}
