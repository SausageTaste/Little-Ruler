#pragma once

#include <optional>
#include <unordered_map>

#include <entt/entity/registry.hpp>

#include "s_threader.h"
#include "u_fileclass.h"
#include "p_meshStatic.h"
#include "u_dlbparser.h"
#include "p_water.h"


namespace dal {

    class ResourceMaster;


    class IModel {

    private:
        ResourceID m_modelResID;
        AABB m_boundingBox;

    public:
        void setModelResID(const ResourceID& resID);
        void setModelResID(ResourceID&& resID);
        const ResourceID& getModelResID(void) const;

        void setBoundingBox(const AABB& box);
        const AABB& getBoundingBox(void) const;

    };

    class ModelStatic : public IModel {

    public:
        struct RenderUnit {
            std::string m_meshName;
            dal::MeshStatic m_mesh;
            dal::Material m_material;
        };

    private:
        std::vector<RenderUnit> m_renderUnits;

    public:
        std::unique_ptr<ICollider> m_bounding, m_detailed;

    public:
        ModelStatic(const ModelStatic&) = delete;
        ModelStatic& operator=(const ModelStatic&) = delete;
        ModelStatic(ModelStatic&&) = delete;
        ModelStatic& operator=(ModelStatic&&) = delete;

    public:
        ModelStatic(void) = default;

        void* operator new(size_t size);
        void operator delete(void* ptr);

        //RenderUnit* addRenderUnit(void);
        void init(const ResourceID& resID, const loadedinfo::ModelStatic& info, ResourceMaster& resMas);
        void init(const loadedinfo::ModelDefined& info, ResourceMaster& resMas);
        
        RenderUnit& addRenderUnit(void) {
            return this->m_renderUnits.emplace_back();
        }

        void invalidate(void);
        bool isReady(void) const;

        void render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf, const glm::mat4& modelMat) const;
        void renderDepthMap(const UniInterfGeometry& unilocGeometry, const glm::mat4& modelMat) const;

    };

    class ModelAnimated : public IModel {

    private:
        struct RenderUnit {
            std::string m_meshName;
            dal::MeshAnimated m_mesh;
            dal::Material m_material;
        };

        std::vector<RenderUnit> m_renderUnits;
        SkeletonInterface m_jointInterface;
        std::vector<Animation> m_animations;
        glm::mat4 m_globalInvMat;

    public:
        void* operator new(size_t size);
        void operator delete(void* ptr);

        RenderUnit* addRenderUnit(void);
        void setSkeletonInterface(SkeletonInterface&& joints);
        void setAnimations(std::vector<Animation>&& animations);
        void setGlobalMat(const glm::mat4 mat);

        bool isReady(void) const;

        void render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf, const UniInterfAnime& unilocAnime,
            const glm::mat4 modelMat, const JointTransformArray& transformArr);
        void renderDepthMap(const UniInterfGeometry& unilocGeometry, const UniInterfAnime& unilocAnime, const glm::mat4 modelMat,
            const JointTransformArray& transformArr) const;

        void invalidate(void);

        //void updateAnimation0(void);

        const SkeletonInterface& getSkeletonInterf(void) const {
            return this->m_jointInterface;
        }
        const std::vector<Animation>& getAnimations(void) const {
            return this->m_animations;
        }
        const glm::mat4& getGlobalInvMat(void) const {
            return this->m_globalInvMat;
        }

    };


    struct ModelStaticHandleImpl;

    class ModelStaticHandle {

    private:
        // This shouldn't be null.
        ModelStaticHandleImpl* m_pimpl;

    public:
        static void* operator new(size_t) = delete;
        static void* operator new[](size_t) = delete;
        static void operator delete(void*) = delete;
        static void operator delete[](void*) = delete;

    public:
        ModelStaticHandle(void);
        explicit ModelStaticHandle(ModelStatic* const model);
        ModelStaticHandle(const ModelStaticHandle&);
        ModelStaticHandle(ModelStaticHandle&&) noexcept;
        ~ModelStaticHandle(void);

        ModelStaticHandle& operator=(const ModelStaticHandle&);
        ModelStaticHandle& operator=(ModelStaticHandle&&) noexcept;

        bool operator==(ModelStaticHandle& other) const;

        void render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf, const glm::mat4& modelMat) const;
        void renderDepthMap(const UniInterfGeometry& unilocGeometry, const glm::mat4& modelMat) const;

        unsigned int getRefCount(void) const;
        const AABB& getBoundingBox(void) const;
        const ResourceID& getResID(void) const;

        const ICollider* getBounding(void) const;
        const ICollider* getDetailed(void) const;

    };


    class MapChunk2 {

    private:
        struct StaticModelActor {
            ModelStaticHandle m_model;
            std::vector<ActorInfo> m_actors;

            StaticModelActor(ModelStaticHandle&& model, std::vector<ActorInfo>&& actors)
                : m_model(std::move(model))
                , m_actors(std::move(actors))
            {

            }
        };

    private:
        std::vector<StaticModelActor> m_staticActors;
        std::vector<WaterRenderer> m_waters;

    public:
        void addStaticActorModel(ModelStaticHandle&& model, std::vector<ActorInfo>&& actors) {
            this->m_staticActors.emplace_back(std::move(model), std::move(actors));
        }
        void addWaterPlane(const dlb::WaterPlane& waterInfo);

        void applyCollision(const ICollider& inCol, cpnt::Transform& inTrans);
        std::optional<RayCastingResult> castRayToClosest(const Ray& ray) const;

        void renderGeneral(const UnilocGeneral& uniloc);
        void renderDepthMp(const UnilocDepthmp& uniloc);
        void renderWater(const UnilocWaterry& uniloc);
        void renderOnWaterGeneral(const UnilocGeneral& uniloc, const ICamera& cam, entt::registry& reg);
        void renderOnWaterAnimated(const UnilocAnimate& uniloc, const ICamera& cam, entt::registry& reg);

    };


    class Package {

    private:
        std::string m_name;
        // All keys are stored in form of file name + ext.
        std::unordered_map<std::string, ModelStaticHandle> m_models;
        std::unordered_map<std::string, ModelAnimated*> m_animatedModels;
        std::unordered_map<std::string, Texture*> m_textures;

    public:
        Package(const Package&) = delete;
        Package& operator=(const Package&) = delete;

    public:
        Package(const std::string& pckName);
        Package(std::string&& pckName);

        Package(Package&& other) noexcept;
        Package& operator=(Package&&) noexcept;
        ~Package(void);

        bool hasTexture(const ResourceID& resPath);
        bool hasModelStatic(const ResourceID& resPath);
        bool hasModelAnim(const ResourceID& resPath);

        std::optional<ModelStaticHandle> getModelStatic(const ResourceID& resID);
        ModelAnimated* getModelAnim(const ResourceID& resID);
        Texture* getTexture(const ResourceID& resID);

        bool giveModelStatic(const ResourceID& resID, ModelStaticHandle mdl);
        bool giveModelAnim(const ResourceID& resID, ModelAnimated* const mdl);
        bool giveTexture(const ResourceID& resID, Texture* const tex);

    };


    class ResourceMaster : public ITaskDoneListener {

        //////// Attribs ////////

    private:
        std::unordered_map<std::string, Package> m_packages;

        //////// Methods ////////

    public:
        ResourceMaster(const ResourceMaster&) = delete;
        ResourceMaster& operator=(const ResourceMaster&) = delete;

    public:
        ResourceMaster(void) = default;
        virtual ~ResourceMaster(void) override = default;

        virtual void notifyTask(std::unique_ptr<ITask> task) override;

        ModelStaticHandle orderModelStatic(const ResourceID& resID);
        ModelAnimated* orderModelAnim(const ResourceID& resID);
        Texture* orderTexture(const ResourceID& resID);
        MapChunk2 loadMap(const ResourceID& resID);

        ModelStaticHandle buildModel(const loadedinfo::ModelDefined& info, const std::string& packageName);

    private:
        Package& orderPackage(const std::string& packName);

    };

}