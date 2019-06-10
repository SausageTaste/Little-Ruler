#include "p_resource.h"

#include <unordered_set>

#include "p_dalopengl.h"
#include "s_logger_god.h"
#include "u_fileclass.h"
#include "s_threader.h"
#include "u_objparser.h"
#include "u_pool.h"


#define BLOCKY_TEXTURE 1


using namespace std::string_literals;


// Tasks
namespace {

    class LoadTask_Texture : public dal::ITask {

    public:
        const dal::ResourceID in_texID;

        dal::loadedinfo::ImageFileData out_img;

        bool out_success = false;

        dal::Texture* data_handle;

    public:
        LoadTask_Texture(const dal::ResourceID& texID, dal::Texture* const handle)
            : in_texID(texID),
            data_handle(handle)
        {

        }

        virtual void start(void) override {
            out_success = dal::futil::getRes_image(in_texID, out_img);
        }

    };


    class LoadTask_Model : public dal::ITask {

    public:
        const dal::ResourceID in_modelID;

        bool out_success;
        dal::loadedinfo::ModelStatic out_info;

        dal::ModelStatic& data_coresponding;
        dal::Package& data_package;

    public:
        LoadTask_Model(const dal::ResourceID& modelID, dal::ModelStatic& coresponding, dal::Package& package)
            : in_modelID(modelID),
            out_success(false),
            data_coresponding(coresponding),
            data_package(package)
        {

        }

        virtual void start(void) override {
            out_success = dal::loadAssimp_staticModel(out_info, this->in_modelID);
        }

    };


    class LoadTask_ModelAnimated : public dal::ITask {

    public:
        const dal::ResourceID in_modelID;

        bool out_success;
        dal::AssimpModelInfo out_info;

        dal::ModelAnimated& data_coresponding;
        dal::Package& data_package;

    public:
        LoadTask_ModelAnimated(const dal::ResourceID& modelID, dal::ModelAnimated& coresponding, dal::Package& package)
            : in_modelID(modelID),
            out_success(false),
            data_coresponding(coresponding),
            data_package(package)
        {

        }

        virtual void start(void) override {
            this->out_success = dal::loadAssimpModel(this->in_modelID, this->out_info, this->data_coresponding);
        }

    };


    std::unordered_set<void*> g_sentTasks_texture;
    std::unordered_set<void*> g_sentTasks_model;
    std::unordered_set<void*> g_sentTasks_modelAnimated;

}


// Pools
namespace {

    dal::StaticPool<dal::ModelStatic, 20> g_modelPool;
    dal::StaticPool<dal::ModelAnimated, 20> g_animatedModelPool;
    dal::StaticPool<dal::Texture, 200> g_texturePool;

}


// Package
namespace dal {

    void Package::ResourceReport::print(void) const {
        dalInfo("Package : "s + m_packageName);
        dalInfo("\tModels");

        for ( auto& x : m_models ) {
            dalInfo("\t\t"s + x.first + " (" + std::to_string(x.second) + ")");
        }

        dalInfo("\tTextures");

        for ( auto& x : m_textures ) {
            dalInfo("\t\t"s + x.first + " (" + std::to_string(x.second) + ")");
        }
    }

    std::string Package::ResourceReport::getStr(void) const {
        std::string result;

        result.append("Package : "s + m_packageName + '\n');
        result.append("\tModels"s + '\n');

        for ( auto& x : m_models ) {
            result.append("\t\t"s + x.first + " (" + std::to_string(x.second) + ")" + '\n');
        }

        result.append("\tTextures"s + '\n');

        for ( auto& x : m_textures ) {
            result.append("\t\t"s + x.first + " (" + std::to_string(x.second) + ")" + '\n');
        }

        return result;
    }

    void Package::setName(const char* const packageName) {
        this->m_name = packageName;
    }

    void Package::setName(const std::string& packageName) {
        this->m_name = packageName;
    }

    ModelStatic* Package::orderModel(const ResourceID& resPath, ResourceMaster* const resMas) {
        std::string modelIDStr{ resPath.makeFileName() };

        decltype(this->m_models.end()) iter = this->m_models.find(modelIDStr);
        if ( this->m_models.end() != iter ) {
            return iter->second.m_data;
        }
        else {
            auto model = g_modelPool.alloc();
            model->setModelResID(resPath);
            this->m_models.emplace(modelIDStr, ManageInfo<ModelStatic>{ model, 2 });

            ResourceID idWithPackage{ this->m_name, resPath.getOptionalDir(), resPath.getBareName(), resPath.getExt() };
            auto task = new LoadTask_Model{ idWithPackage, *model, *this };
            g_sentTasks_model.insert(task);
            TaskGod::getinst().orderTask(task, resMas);

            return model;
        }
    }

    ModelAnimated* Package::orderModelAnimated(const ResourceID& resPath, ResourceMaster* const resMas) {
        std::string modelIDStr{ resPath.makeFileName() };

        auto iter = this->m_animatedModels.find(modelIDStr);
        if ( this->m_animatedModels.end() != iter ) {
            return iter->second.m_data;
        }
        else {
            auto model = g_animatedModelPool.alloc();
            model->setModelResID(resPath);
            this->m_animatedModels.emplace(modelIDStr, ManageInfo<ModelAnimated>{ model, 2 });

            ResourceID idWithPackage{ this->m_name, resPath.getOptionalDir(), resPath.getBareName(), resPath.getExt() };
            auto task = new LoadTask_ModelAnimated{ idWithPackage, *model, *this };
            g_sentTasks_modelAnimated.insert(task);
            TaskGod::getinst().orderTask(task, resMas);

            return model;
        }
    }

    ModelStatic* Package::buildModel(const loadedinfo::ModelDefined& info, ResourceMaster* const resMas) {
        auto model = g_modelPool.alloc();
        this->m_models.emplace(info.m_modelID, ManageInfo<ModelStatic>{ model, 1 });

        // Bounding box
        model->setBoundingBox(info.m_boundingBox);

        {
            // Render units
            auto renderUnit = model->addRenderUnit();
            renderUnit->m_mesh.buildData(
                info.m_renderUnit.m_mesh.m_vertices.data(), info.m_renderUnit.m_mesh.m_vertices.size(),
                info.m_renderUnit.m_mesh.m_texcoords.data(), info.m_renderUnit.m_mesh.m_texcoords.size(),
                info.m_renderUnit.m_mesh.m_normals.data(), info.m_renderUnit.m_mesh.m_normals.size()
            );

            // Material
            renderUnit->m_material.m_specularStrength = info.m_renderUnit.m_material.m_specStrength;
            renderUnit->m_material.m_shininess = info.m_renderUnit.m_material.m_shininess;
            renderUnit->m_material.m_diffuseColor = info.m_renderUnit.m_material.m_diffuseColor;
            renderUnit->m_material.setTexScale(info.m_renderUnit.m_material.m_texSize.x, info.m_renderUnit.m_material.m_texSize.y);

            if ( !info.m_renderUnit.m_material.m_diffuseMap.empty() ) {
                auto texHandle = this->orderDiffuseMap(info.m_renderUnit.m_material.m_diffuseMap.c_str(), resMas);
                renderUnit->m_material.setDiffuseMap(texHandle);
            }

        }

        return model;
    }

    Texture* Package::orderDiffuseMap(const ResourceID& texID, ResourceMaster* const resMas) {
        auto iter = this->m_textures.find(texID.makeFileName());
        if ( this->m_textures.end() == iter ) {
            auto texture = g_texturePool.alloc();
            this->m_textures.emplace(texID.makeFileName(), ManageInfo<Texture>{ texture, 2 });  // ref count is 2 because of return and task.

            ResourceID idWithPackage{ this->m_name, texID.getOptionalDir(), texID.getBareName(), texID.getExt() };
            auto task = new LoadTask_Texture(idWithPackage, texture);
            g_sentTasks_texture.insert(task);
            TaskGod::getinst().orderTask(task, resMas);

            return texture;
        }
        else {
            iter->second.m_refCount++;
            return iter->second.m_data;
        }
    }

    Texture* Package::buildDiffuseMap(const ResourceID& texID, const loadedinfo::ImageFileData& info) {
        auto tex = g_texturePool.alloc();

        if ( 3 == info.m_pixSize ) {
            tex->init_diffueMap3(info.m_buf.data(), info.m_width, info.m_height);
        }
        else if ( 4 == info.m_pixSize ) {
            tex->init_diffueMap(info.m_buf.data(), info.m_width, info.m_height);
        }
        else {
            dalError("Not supported pixel size: "s + texID.makeIDStr() + ", " + std::to_string(info.m_pixSize));
        }

        this->m_textures.emplace(texID.makeFileName(), ManageInfo<Texture>{ tex, 1 });
        return tex;
    }

    void Package::getResReport(ResourceReport& report) const {
        report.m_packageName = this->m_name;

        report.m_models.clear();
        report.m_models.reserve(this->m_models.size());
        for ( auto& x : this->m_models ) {
            report.m_models.emplace_back(x.first, x.second.m_refCount);
        }

        report.m_textures.clear();
        report.m_textures.reserve(this->m_textures.size());
        for ( auto& x : this->m_textures ) {
            report.m_textures.emplace_back(x.first, x.second.m_refCount);
        }
    }

    void Package::clear(void) {
        for ( auto& modelPair : this->m_models ) {
            modelPair.second.m_data->destroyModel();
            g_modelPool.free(modelPair.second.m_data);
        }
        this->m_models.clear();

        for ( auto& tex : this->m_textures ) {
            tex.second.m_data->deleteTex();
            g_texturePool.free(tex.second.m_data);
        }
        this->m_textures.clear();
    }

}


// ResourceMaster
namespace dal {

    ResourceMaster::~ResourceMaster(void) {
        for ( auto& package : this->m_packages ) {
            package.second.clear();
        }
        this->m_packages.clear();
    }

    void ResourceMaster::notifyTask(std::unique_ptr<ITask> task) {
        if ( nullptr == task ) {
            dalAbort("ResourceMaster::notifyTask has got a nullptr. Why??");
        }

        if ( g_sentTasks_model.find(task.get()) != g_sentTasks_model.end() ) {
            g_sentTasks_model.erase(task.get());

            auto loaded = reinterpret_cast<LoadTask_Model*>(task.get());
            if ( !loaded->out_success ) {
                dalError("Failed to load model: "s + loaded->in_modelID.makeIDStr());
                return;
            }

            loaded->data_coresponding.setBoundingBox(loaded->out_info.m_aabb);

            for ( auto& unitInfo : loaded->out_info.m_renderUnits ) {
                auto unit = loaded->data_coresponding.addRenderUnit();
                assert(nullptr != unit);

                unit->m_mesh.buildData(
                    unitInfo.m_mesh.m_vertices.data(), unitInfo.m_mesh.m_vertices.size(),
                    unitInfo.m_mesh.m_texcoords.data(), unitInfo.m_mesh.m_texcoords.size(),
                    unitInfo.m_mesh.m_normals.data(), unitInfo.m_mesh.m_normals.size()
                );
                unit->m_meshName = unitInfo.m_name;

                unit->m_material.m_diffuseColor = unitInfo.m_material.m_diffuseColor;
                unit->m_material.m_shininess = unitInfo.m_material.m_shininess;
                unit->m_material.m_specularStrength = unitInfo.m_material.m_specStrength;

                if ( !unitInfo.m_material.m_diffuseMap.empty() ) {
                    auto tex = loaded->data_package.orderDiffuseMap(unitInfo.m_material.m_diffuseMap, this);
                    unit->m_material.setDiffuseMap(tex);
                }
            }
        }
        else if ( g_sentTasks_texture.find(task.get()) != g_sentTasks_texture.end() ) {
            g_sentTasks_texture.erase(task.get());

            auto loaded = reinterpret_cast<LoadTask_Texture*>(task.get());
            if ( !loaded->out_success ) {
                dalError("Failed to load texture: "s + loaded->in_texID.makeIDStr());
                return;
            }

            if ( loaded->out_img.m_pixSize == 3 ) {
                loaded->data_handle->init_diffueMap3(loaded->out_img.m_buf.data(), loaded->out_img.m_width, loaded->out_img.m_height);
            }
            else if ( loaded->out_img.m_pixSize == 4 ) {
                loaded->data_handle->init_diffueMap(loaded->out_img.m_buf.data(), loaded->out_img.m_width, loaded->out_img.m_height);
            }
            else {
                dalAbort("Unknown pix size: "s + std::to_string(loaded->out_img.m_pixSize));
            }
        }
        else if ( g_sentTasks_modelAnimated.find(task.get()) != g_sentTasks_modelAnimated.end() ) {
            g_sentTasks_modelAnimated.erase(task.get());

            auto loaded = reinterpret_cast<LoadTask_ModelAnimated*>(task.get());
            if ( !loaded->out_success ) {
                dalError("Failed to load model: "s + loaded->in_modelID.makeIDStr());
                return;
            }

            loaded->data_coresponding.setBoundingBox(loaded->out_info.m_model.m_aabb);
            loaded->data_coresponding.setSkeletonInterface(std::move(loaded->out_info.m_model.m_joints));
            loaded->data_coresponding.setAnimations(std::move(loaded->out_info.m_animations));

            for ( auto& unitInfo : loaded->out_info.m_model.m_renderUnits ) {
                auto unit = loaded->data_coresponding.addRenderUnit();
                assert(nullptr != unit);

                unit->m_mesh.buildData(
                    unitInfo.m_mesh.m_vertices.data(),
                    unitInfo.m_mesh.m_texcoords.data(),
                    unitInfo.m_mesh.m_normals.data(),
                    unitInfo.m_mesh.m_boneIndex.data(),
                    unitInfo.m_mesh.m_boneWeights.data(),
                    unitInfo.m_mesh.m_vertices.size() / 3
                );
                unit->m_meshName = unitInfo.m_name;

                unit->m_material.m_diffuseColor = unitInfo.m_material.m_diffuseColor;
                unit->m_material.m_shininess = unitInfo.m_material.m_shininess;
                unit->m_material.m_specularStrength = unitInfo.m_material.m_specStrength;

                if ( !unitInfo.m_material.m_diffuseMap.empty() ) {
                    auto tex = loaded->data_package.orderDiffuseMap(unitInfo.m_material.m_diffuseMap, this);
                    unit->m_material.setDiffuseMap(tex);
                }
            }
        }
        else {
            dalWarn("ResourceMaster got a task that it doesn't know.");
        }
    }

    ModelStatic* ResourceMaster::orderModel(const ResourceID& resID) {
        auto& package = this->orderPackage(resID.getPackage());

        return package.orderModel(resID, this);
    }

    ModelAnimated* ResourceMaster::orderModelAnimated(const ResourceID& resID) {
        auto& package = this->orderPackage(resID.getPackage());

        return package.orderModelAnimated(resID, this);
    }

    ModelStatic* ResourceMaster::buildModel(const loadedinfo::ModelDefined& info, const char* const packageName) {
        auto& package = this->orderPackage(packageName);
        return package.buildModel(info, this);
    }

    size_t ResourceMaster::getResReports(std::vector<Package::ResourceReport>& reports) const {
        reports.clear();
        reports.resize(this->m_packages.size());

        int i = 0;
        for ( auto& x : this->m_packages ) {
            x.second.getResReport(reports[i]);
            i++;
        }

        return reports.size();
    }

    // Static

    Texture* ResourceMaster::getUniqueTexture(void) {
        return g_texturePool.alloc();
    }

    void ResourceMaster::dumpUniqueTexture(Texture* const tex) {
        g_texturePool.free(tex);
    }

    // Private

    Package& ResourceMaster::orderPackage(const std::string& packName) {
        std::string packNameStr{ packName };

        decltype(this->m_packages.end()) iter = this->m_packages.find(packNameStr);
        if ( iter != this->m_packages.end() ) {
            return iter->second;
        }
        else { // If not found
            Package package;
            package.setName(packName);
            auto res = this->m_packages.emplace(packName, package);
            return res.first->second;
        }
    }

}