#pragma once

#include <optional>
#include <unordered_map>

#include "s_threader.h"
#include "u_fileclass.h"
#include "p_meshStatic.h"


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
        ModelStaticHandle buildModel(const loadedinfo::ModelDefined& info, const std::string& packageName);

    private:
        Package& orderPackage(const std::string& packName);

    };

}