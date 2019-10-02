#pragma once

#include <optional>
#include <iostream>
#include <unordered_map>
#include <memory>

#include <entt/entity/registry.hpp>

#include "u_fileclass.h"
#include "p_meshStatic.h"


namespace dal {

    template <typename _MeshTyp>
    class IModel {

    public:
        template <typename _MeshTyp>
        struct RenderUnit {
            std::string m_name;
            _MeshTyp m_mesh;
            dal::Material m_material;
        };

    private:
        ResourceID m_resID;
        std::unique_ptr<ICollider> m_bounding, m_detailed;
    protected:
        std::vector<RenderUnit<_MeshTyp>> m_renderUnits;

    public:
        IModel(const IModel&) = delete;
        IModel& operator=(const IModel&) = delete;

        IModel(void) = default;
        IModel(IModel&&) = default;
        IModel& operator=(IModel&&) = default;

        void clearRenderUnits(void) {
            this->m_renderUnits.clear();
        }
        void reserveRenderUnits(const size_t size) {
            this->m_renderUnits.reserve(size);
        }
        RenderUnit<_MeshTyp>& newRenderUnit(void) {
            return this->m_renderUnits.emplace_back();
        }

        void setResID(const ResourceID& m_resID) {
            this->m_resID = m_resID;
        }
        const ResourceID& getResID(void) const {
            return this->m_resID;
        }

        void setBounding(std::unique_ptr<ICollider>&& col) {
            this->m_bounding = std::move(col);
        }
        const ICollider* getBounding(void) const {
            return this->m_bounding.get();
        }

        void setDetailed(std::unique_ptr<ICollider>&& col) {
            this->m_detailed = std::move(col);
        }
        const ICollider* getDetailed(void) const {
            return this->m_detailed.get();
        }

    };

    class ModelStatic : public IModel<MeshStatic> {

    public:
        void* operator new(size_t size);
        void operator delete(void* ptr);

        bool isReady(void) const;

        void render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf, const glm::mat4& modelMat) const;
        void renderDepth(const UniInterfGeometry& unilocGeometry, const glm::mat4& modelMat) const;

    };

    class ModelAnimated : public IModel<MeshAnimated> {

    private:
        SkeletonInterface m_jointInterface;
        std::vector<Animation> m_animations;
        glm::mat4 m_globalInvMat;

    public:
        void* operator new(size_t size);
        void operator delete(void* ptr);

        void setSkeletonInterface(SkeletonInterface&& joints);
        void setAnimations(std::vector<Animation>&& animations);
        void setGlobalMat(const glm::mat4 mat);

        bool isReady(void) const;

        void render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf, const UniInterfAnime& unilocAnime,
            const glm::mat4 modelMat, const JointTransformArray& transformArr);
        void renderDepth(const UniInterfGeometry& unilocGeometry, const UniInterfAnime& unilocAnime, const glm::mat4 modelMat,
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


    /*
    template <typename _Model>
    struct ModelHandleImpl {
        std::unique_ptr<_Model> m_model;
        size_t m_refCount = 1;
    };

    template <typename _Model>
    class IModelHandle {

    private:
        ModelHandleImpl<_Model>* m_pimpl;

    public:
        static void* operator new(size_t) = delete;
        static void* operator new[](size_t) = delete;
        static void operator delete(void*) = delete;
        static void operator delete[](void*) = delete;

    public:
        IModelHandle(void)
            : m_pimpl(new ModelHandleImpl<_Model>)
        {

        }
        IModelHandle(_Model* const pModel)
            : m_pimpl(new ModelHandleImpl<_Model>)
        {
            this->m_pimpl->m_model.reset(pModel);
        }
        ~IModelHandle(void) {
            if ( nullptr == this->m_pimpl ) {
                return;
            }
            else {
                this->removeOneRef(this->m_pimpl);
                this->m_pimpl = nullptr;
            }
        }

        IModelHandle(const IModelHandle& other)
            : m_pimpl(other.m_pimpl)
        {
            ++this->m_pimpl->m_refCount;
        }
        IModelHandle(IModelHandle&& other) noexcept
            : m_pimpl(nullptr)
        {
            std::swap(this->m_pimpl, other.m_pimpl);
        }
        IModelHandle& operator=(const IModelHandle& other) {
            this->removeOneRef(this->m_pimpl);
            this->m_pimpl = other.m_pimpl;
            ++this->m_pimpl->m_refCount;
            return *this;
        }
        IModelHandle& operator=(IModelHandle&& other) noexcept {
            std::swap(this->m_pimpl, other.m_pimpl);
            return *this;
        }

        bool operator==(const IModelHandle<_Model>& other) const {
            if ( nullptr == this->m_pimpl->m_model ) {
                return false;
            }
            else if ( nullptr == other.m_pimpl->m_model ) {
                return false;
            }
            else if ( this->m_pimpl->m_model != other.m_pimpl->m_model ) {
                return false;
            }
            else {
                return true;
            }
        }

        size_t getRefCount(void) const {
            return this->m_pimpl->m_refCount;
        }
        const ResourceID& getResID(void) const {
            return this->getPimpl()->m_model->getResID();
        }
        const ICollider* getBounding(void) const {
            return &this->getPimpl()->m_model->getBounding();
        }
        const ICollider* getDetailed(void) const {
            return &this->getPimpl()->m_model->getDetailed();
        }

        void reset(_Model* const model) {
            this->m_pimpl->m_model.reset(model);
        }

    protected:
        ModelHandleImpl<_Model>* getPimpl(void) {
            return this->m_pimpl;
        }
        const ModelHandleImpl<_Model>* getPimpl(void) const {
            return this->m_pimpl;
        }

    private:
        void removeOneRef(ModelHandleImpl<_Model>* const pimpl) const {
            --pimpl->m_refCount;
            if ( 0 == pimpl->m_refCount ) {
                delete pimpl;
            }
        }

    };
    */


    /*
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
    */


    /*
    class ModelStaticHandle : public IModelHandle<ModelStatic> {

    public:
        void render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf, const glm::mat4& modelMat) const;
        void renderDepth(const UniInterfGeometry& unilocGeometry, const glm::mat4& modelMat) const;

    };

    class ModelAnimatedHandle : public IModelHandle<ModelAnimated> {

    public:
        const SkeletonInterface& getSkeletonInterf(void) const;
        const std::vector<Animation>& getAnimations(void) const;
        const glm::mat4& getGlobalInvMat(void) const;

        void render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf, const UniInterfAnime& unilocAnime,
            const glm::mat4 modelMat, const JointTransformArray& transformArr);
        void renderDepth(const UniInterfGeometry& unilocGeometry, const UniInterfAnime& unilocAnime, const glm::mat4 modelMat,
            const JointTransformArray& transformArr) const;

    };
    */


    using ModelStaticHandle = std::shared_ptr<ModelStatic>;
    using ModelAnimatedHandle = std::shared_ptr<ModelAnimated>;


    namespace cpnt {

        struct StaticModel {
            ModelStaticHandle m_model;
        };

        struct AnimatedModel {
            ModelAnimatedHandle m_model;
            AnimationState m_animState;
        };

    }

}
