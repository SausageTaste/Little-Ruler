#pragma once

#include <optional>
#include <unordered_map>

#include <entt/entity/registry.hpp>

#include "u_fileclass.h"
#include "p_meshStatic.h"
#include "p_water.h"


namespace dal {

    class IModel {

    private:
        ResourceID m_modelResID;

    public:
        void setModelResID(const ResourceID& resID);
        void setModelResID(ResourceID&& resID);
        const ResourceID& getModelResID(void) const;

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
        //void init(const ResourceID& resID, const loadedinfo::ModelStatic& info, ResourceMaster& resMas);
        //void init(const loadedinfo::ModelDefined& info, ResourceMaster& resMas);
        
        RenderUnit& newRenderUnit(void) {
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
        const ResourceID& getResID(void) const;

        const ICollider* getBounding(void) const;
        const ICollider* getDetailed(void) const;

    };

}