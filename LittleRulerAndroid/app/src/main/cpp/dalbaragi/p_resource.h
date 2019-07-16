#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

#include "u_loadinfo.h"
#include "p_uniloc.h"
#include "s_threader.h"
#include "u_fileclass.h"
#include "p_meshStatic.h"
#include "u_timer.h"
#include "p_animation.h"


namespace dal {

    class ResourceMaster;


    class IModel {

    private:
        ResourceID m_modelResID;
        AxisAlignedBoundingBox m_boundingBox;

    public:
        void setModelResID(const ResourceID& resID);
        void setModelResID(ResourceID&& resID);
        const ResourceID& getModelResID(void) const;

        void setBoundingBox(const AxisAlignedBoundingBox& box);
        const AxisAlignedBoundingBox& getBoundingBox(void) const;

    };

    class ModelStatic : public IModel {

    private:
        struct RenderUnit {
            std::string m_meshName;
            dal::MeshStatic m_mesh;
            dal::Material m_material;
        };

        std::vector<RenderUnit> m_renderUnits;

    public:
        ModelStatic(const ModelStatic&) = delete;
        ModelStatic& operator=(const ModelStatic&) = delete;
        ModelStatic(ModelStatic&&) = delete;
        ModelStatic& operator=(ModelStatic&&) = delete;

    public:
        ModelStatic(void) = default;
        ~ModelStatic(void);

        //RenderUnit* addRenderUnit(void);
        void init(const ResourceID& resID, const loadedinfo::ModelStatic& info, ResourceMaster& resMas);
        void init(const loadedinfo::ModelDefined& info, ResourceMaster& resMas);

        bool isReady(void) const;

        void render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf, const glm::mat4& modelMat) const;
        void renderDepthMap(const UniInterfGeometry& unilocGeometry, const glm::mat4& modelMat) const;

        void destroyModel(void);

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
        RenderUnit* addRenderUnit(void);
        void setSkeletonInterface(SkeletonInterface&& joints);
        void setAnimations(std::vector<Animation>&& animations);
        void setGlobalMat(const glm::mat4 mat);

        bool isReady(void) const;

        void render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf, const UniInterfAnime& unilocAnime,
            const glm::mat4 modelMat, const JointTransformArray& transformArr);
        void renderDepthMap(const UniInterfGeometry& unilocGeometry, const UniInterfAnime& unilocAnime, const glm::mat4 modelMat,
            const JointTransformArray& transformArr) const;

        void destroyModel(void);

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
        const AxisAlignedBoundingBox& getBoundingBox(void) const;
        const ResourceID& getResID(void) const;

    };


    class Package {

    private:
        template <typename T>
        struct ManageInfo {
            T* m_data = nullptr;
            int64_t m_refCount = 0;
        };

    private:
        std::string m_name;
        std::unordered_map<std::string, ModelStaticHandle> m_models;
        std::unordered_map<std::string, ManageInfo<ModelAnimated>> m_animatedModels;
        std::unordered_map<std::string, ManageInfo<Texture>> m_textures;

    public:
        void setName(const char* const packageName);
        void setName(const std::string& packageName);

        ModelStaticHandle orderModel(const ResourceID& resPath, ResourceMaster* const resMas);
        ModelAnimated* orderModelAnimated(const ResourceID& resPath, ResourceMaster* const resMas);
        ModelStaticHandle buildModel(const loadedinfo::ModelDefined& info, ResourceMaster* const resMas);
        Texture* orderDiffuseMap(ResourceID texID, ResourceMaster* const resMas);

        void clear(void);

    private:
        Texture* buildDiffuseMap(const ResourceID& texID, const loadedinfo::ImageFileData& info);

    };


    class ResourceMaster : public ITaskDoneListener {

        //////// Attribs ////////

    private:
        std::unordered_map<std::string, Package> m_packages;

        //////// Methods ////////

    public:
        virtual ~ResourceMaster(void) override;

        virtual void notifyTask(std::unique_ptr<ITask> task) override;

        ModelStaticHandle orderModel(const ResourceID& resID);
        ModelAnimated* orderModelAnimated(const ResourceID& resID);
        ModelStaticHandle buildModel(const loadedinfo::ModelDefined& info, const char* const packageName);

        Texture* orderTexture(const ResourceID& resID);

        static Texture* getUniqueTexture(void);
        static void dumpUniqueTexture(Texture* const tex);

    private:
        Package& orderPackage(const std::string& packName);

    };

}