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
        Timer m_animLocalTimer;

    public:
        RenderUnit* addRenderUnit(void);
        void setSkeletonInterface(SkeletonInterface&& joints);
        void setAnimations(std::vector<Animation>&& animations);
        void setGlobalMat(const glm::mat4 mat);

        bool isReady(void) const;

        void render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf, const UniInterfAnime& unilocAnime,
            const glm::mat4 modelMat);
        void renderDepthMap(const UniInterfGeometry& unilocGeometry, const UniInterfAnime& unilocAnime, const glm::mat4 modelMat) const;

        void destroyModel(void);

        void updateAnimation0(void);

    };


    class ModelStaticHandle {

    private:
        struct Impl;

    private:
        Impl* pimpl;

    public:
        ModelStaticHandle(void);
        ~ModelStaticHandle(void);

        ModelStaticHandle(const ModelStaticHandle&) = delete;
        ModelStaticHandle& operator=(const ModelStaticHandle&) = delete;

        ModelStaticHandle(ModelStaticHandle&&) noexcept;
        ModelStaticHandle& operator=(ModelStaticHandle&&) noexcept;

        void render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf, const glm::mat4& modelMat) const;
        void renderDepthMap(const UniInterfGeometry& unilocGeometry, const glm::mat4& modelMat) const;

    };


    class Package {

    public:
        struct ResourceReport {
            std::string m_packageName;
            std::vector<std::pair<std::string, unsigned int>> m_models;
            std::vector<std::pair<std::string, unsigned int>> m_textures;

            void print(void) const;
            std::string getStr(void) const;
        };

    private:
        template <typename T>
        struct ManageInfo {
            T* m_data = nullptr;
            int64_t m_refCount = 0;
        };

    private:
        std::string m_name;
        std::unordered_map<std::string, ManageInfo<ModelStatic>> m_models;
        std::unordered_map<std::string, ManageInfo<ModelAnimated>> m_animatedModels;
        std::unordered_map<std::string, ManageInfo<Texture>> m_textures;

    public:
        void setName(const char* const packageName);
        void setName(const std::string& packageName);

        ModelStatic* orderModel(const ResourceID& resPath, ResourceMaster* const resMas);
        ModelAnimated* orderModelAnimated(const ResourceID& resPath, ResourceMaster* const resMas);
        ModelStatic* buildModel(const loadedinfo::ModelDefined& info, ResourceMaster* const resMas);
        Texture* orderDiffuseMap(ResourceID texID, ResourceMaster* const resMas);

        void getResReport(ResourceReport& report) const;

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

        ModelStatic* orderModel(const ResourceID& resID);
        ModelAnimated* orderModelAnimated(const ResourceID& resID);
        ModelStatic* buildModel(const loadedinfo::ModelDefined& info, const char* const packageName);

        Texture* orderTexture(const ResourceID& resID);

        static Texture* getUniqueTexture(void);
        static void dumpUniqueTexture(Texture* const tex);

        size_t getResReports(std::vector<Package::ResourceReport>& reports) const;

    private:
        Package& orderPackage(const std::string& packName);

    };

}