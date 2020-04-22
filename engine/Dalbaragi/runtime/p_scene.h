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
            glm::vec3 m_offsetPos;
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

        //////// Attribs ////////

    private:
        ResourceMaster& m_resMas;
        PhysicsWorld& m_phyworld;

        LevelData m_activeLevel;
        std::list<MapChunk2> m_mapChunks2;

    public:
        entt::registry m_entities;
        entt::entity m_player;
        StrangeEulerCamera m_playerCam;

        //////// Methods ////////

    public:
        SceneGraph(ResourceMaster& resMas, PhysicsWorld& phyworld, const unsigned int winWidth, const unsigned int winHeight);

        void update(const float deltaTime);

        entt::entity addObj_static(const char* const resid);

        void renderWater(const UnilocWaterry& uniloc);

        void render_static(const UniRender_Static& uniloc);
        void render_animated(const UniRender_Animated& uniloc);
        void render_staticDepth(const UniRender_StaticDepth& uniloc);
        void render_animatedDepth(const UniRender_AnimatedDepth& uniloc);

        std::vector<WaterRenderer*> waters(void);

        void applyCollision(const ICollider& inCol, cpnt::Transform& inTrans);
        std::optional<RayCastingResult> doRayCasting(const Segment& ray);

        void onResize(const unsigned int width, const unsigned int height);

    private:
        void openLevel(const char* const respath);
        void openChunk(const char* const respath);

    };

}
