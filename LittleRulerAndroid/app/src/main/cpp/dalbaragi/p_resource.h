#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

#include "u_loadinfo.h"
#include "p_uniloc.h"
#include "s_threader.h"
#include "u_fileclass.h"
#include "p_model.h"


namespace dal {

    class ResourceMaster;


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
        std::unordered_map<std::string, ManageInfo<Model>> m_models;
        std::unordered_map<std::string, ManageInfo<ModelAnimated>> m_animatedModels;
        std::unordered_map<std::string, ManageInfo<Texture>> m_textures;

    public:
        void setName(const char* const packageName);
        void setName(const std::string& packageName);

        Model* orderModel(const ResourceID& resPath, ResourceMaster* const resMas);
        ModelAnimated* orderModelAnimated(const ResourceID& resPath, ResourceMaster* const resMas);
        Model* buildModel(const loadedinfo::ModelDefined& info, ResourceMaster* const resMas);
        Texture* orderDiffuseMap(const ResourceID& texID, ResourceMaster* const resMas);

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

        Model* orderModel(const ResourceID& resID);
        ModelAnimated* orderModelAnimated(const ResourceID& resID);
        Model* buildModel(const loadedinfo::ModelDefined& info, const char* const packageName);

        static Texture* getUniqueTexture(void);
        static void dumpUniqueTexture(Texture* const tex);

        size_t getResReports(std::vector<Package::ResourceReport>& reports) const;

    private:
        Package& orderPackage(const std::string& packName);

    };

}