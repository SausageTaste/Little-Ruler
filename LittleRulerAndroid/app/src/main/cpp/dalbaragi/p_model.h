#pragma once

#include <memory>

#include <entt/entity/registry.hpp>

#include "p_meshStatic.h"


namespace dal {

    template <typename _MeshTyp>
    class IModel {

    public:
        template <typename _MeshTypInner>
        struct RenderUnit {
            std::string m_name;
            _MeshTypInner m_mesh;
            dal::Material m_material;
        };

    private:
        std::string m_resID;
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

        void setResID(const std::string& m_resID) {
            this->m_resID = m_resID;
        }
        const std::string& getResID(void) const {
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

        void render(const UniInterfLightedMesh& unilocLighted, const UniInterfLightmaps& unilocLightmaps, const glm::mat4& modelMat) const;
        void render(const UniRender_Static& uniloc) const;
        void renderDepth(const UniInterfGeometry& unilocGeometry, const glm::mat4& modelMat) const;

    };


    class ModelAnimated : public IModel<MeshAnimated> {

    private:
        SkeletonInterface m_jointInterface;
        std::vector<Animation> m_animations;

    public:
        void* operator new(size_t size);
        void operator delete(void* ptr);

        void setSkeletonInterface(SkeletonInterface&& joints);
        void setAnimations(std::vector<Animation>&& animations);

        bool isReady(void) const;

        void render(const UniInterfLightedMesh& unilocLighted, const UniInterfLightmaps& unilocLightmaps, const UniInterfAnime& unilocAnime,
            const glm::mat4 modelMat, const JointTransformArray& transformArr) const;
        void renderDepth(const UniInterfGeometry& unilocGeometry, const UniInterfAnime& unilocAnime, const glm::mat4 modelMat,
            const JointTransformArray& transformArr) const;

        const SkeletonInterface& getSkeletonInterf(void) const {
            return this->m_jointInterface;
        }
        const std::vector<Animation>& getAnimations(void) const {
            return this->m_animations;
        }

    };


    namespace cpnt {

        struct StaticModel {
            std::shared_ptr<const ModelStatic> m_model;
        };

        struct AnimatedModel {
            std::shared_ptr<const ModelAnimated> m_model;
            AnimationState m_animState;
        };

    }

}
