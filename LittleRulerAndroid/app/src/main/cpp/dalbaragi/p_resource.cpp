#include "p_resource.h"

#include <unordered_set>

#include <glm/gtc/matrix_transform.hpp>
#include <assimp/matrix4x4.h>

#include "p_dalopengl.h"
#include "s_logger_god.h"
#include "u_fileclass.h"
#include "s_threader.h"
#include "u_objparser.h"
#include "u_pool.h"


#define BLOCKY_TEXTURE 0


using namespace std::string_literals;


namespace {

    inline glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4* from) {
        glm::mat4 to;
        using GLfloat = float;

        to[0][0] = (GLfloat)from->a1; to[0][1] = (GLfloat)from->b1;  to[0][2] = (GLfloat)from->c1; to[0][3] = (GLfloat)from->d1;
        to[1][0] = (GLfloat)from->a2; to[1][1] = (GLfloat)from->b2;  to[1][2] = (GLfloat)from->c2; to[1][3] = (GLfloat)from->d2;
        to[2][0] = (GLfloat)from->a3; to[2][1] = (GLfloat)from->b3;  to[2][2] = (GLfloat)from->c3; to[2][3] = (GLfloat)from->d3;
        to[3][0] = (GLfloat)from->a4; to[3][1] = (GLfloat)from->b4;  to[3][2] = (GLfloat)from->c4; to[3][3] = (GLfloat)from->d4;

        return to;
    }

}


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
            : in_texID(texID)
            , data_handle(handle)
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
            this->out_success = dal::loadAssimpModel(this->in_modelID, this->out_info);
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





// IModel
namespace dal {

    void IModel::setModelResID(const ResourceID& resID) {
        this->m_modelResID = resID;
    }

    void IModel::setModelResID(ResourceID&& resID) {
        this->m_modelResID = std::move(resID);
    }

    const ResourceID& IModel::getModelResID(void) const {
        return this->m_modelResID;
    }

    void IModel::setBoundingBox(const AxisAlignedBoundingBox& box) {
        this->m_boundingBox = box;
    }

    const AxisAlignedBoundingBox& IModel::getBoundingBox(void) const {
        return this->m_boundingBox;
    }

}


// ModelStatic
namespace dal {

    ModelStatic::~ModelStatic(void) {
        this->destroyModel();
    }

    void ModelStatic::init(const ResourceID& resID, const loadedinfo::ModelStatic& info, ResourceMaster& resMas) {
        this->setModelResID(std::move(resID));
        this->setBoundingBox(info.m_aabb);

        for ( auto& unitInfo : info.m_renderUnits ) {
            auto& unit = this->m_renderUnits.emplace_back();

            unit.m_mesh.buildData(
                unitInfo.m_mesh.m_vertices.data(), unitInfo.m_mesh.m_vertices.size(),
                unitInfo.m_mesh.m_texcoords.data(), unitInfo.m_mesh.m_texcoords.size(),
                unitInfo.m_mesh.m_normals.data(), unitInfo.m_mesh.m_normals.size()
            );
            unit.m_meshName = unitInfo.m_name;

            unit.m_material.m_diffuseColor = unitInfo.m_material.m_diffuseColor;
            unit.m_material.m_shininess = unitInfo.m_material.m_shininess;
            unit.m_material.m_specularStrength = unitInfo.m_material.m_specStrength;

            if ( !unitInfo.m_material.m_diffuseMap.empty() ) {
                ResourceID texResID{ unitInfo.m_material.m_diffuseMap };
                texResID.setPackageIfEmpty(resID.getPackage());
                auto tex = resMas.orderTexture(texResID);
                unit.m_material.setDiffuseMap(tex);
            }
        }
    }

    void ModelStatic::init(const loadedinfo::ModelDefined& info, ResourceMaster& resMas) {
        this->setBoundingBox(info.m_boundingBox);

        auto& unitInfo = info.m_renderUnit;
        auto& unit = this->m_renderUnits.emplace_back();

        unit.m_mesh.buildData(
            unitInfo.m_mesh.m_vertices.data(), unitInfo.m_mesh.m_vertices.size(),
            unitInfo.m_mesh.m_texcoords.data(), unitInfo.m_mesh.m_texcoords.size(),
            unitInfo.m_mesh.m_normals.data(), unitInfo.m_mesh.m_normals.size()
        );
        unit.m_meshName = unitInfo.m_name;

        unit.m_material.m_diffuseColor = unitInfo.m_material.m_diffuseColor;
        unit.m_material.m_shininess = unitInfo.m_material.m_shininess;
        unit.m_material.m_specularStrength = unitInfo.m_material.m_specStrength;
        unit.m_material.setTexScale(info.m_renderUnit.m_material.m_texSize.x, info.m_renderUnit.m_material.m_texSize.y);

        if ( !unitInfo.m_material.m_diffuseMap.empty() ) {
            auto tex = resMas.orderTexture(unitInfo.m_material.m_diffuseMap);
            unit.m_material.setDiffuseMap(tex);
        }
    }

    bool ModelStatic::isReady(void) const {
        for ( const auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) return false;
        }

        return true;
    }

    void ModelStatic::render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf, const glm::mat4& modelMat) const {
        if ( !this->isReady() ) {
            return;
        }

        for ( auto& unit : this->m_renderUnits ) {
            unit.m_material.sendUniform(unilocLighted, samplerInterf);
            if ( !unit.m_mesh.isReady() ) {
                continue;
            }

            unilocLighted.modelMat(modelMat);
            unit.m_mesh.draw();
        }
    }

    void ModelStatic::renderDepthMap(const UniInterfGeometry& unilocGeometry, const glm::mat4& modelMat) const {
        if ( !this->isReady() ) {
            return;
        }

        for ( auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) {
                continue;
            }

            unilocGeometry.modelMat(modelMat);
            unit.m_mesh.draw();
        }
    }

    void ModelStatic::destroyModel(void) {
        for ( auto& unit : this->m_renderUnits ) {
            unit.m_mesh.destroyData();
        }
        this->m_renderUnits.clear();
    }

}


// ModelAnimated
namespace dal {

    ModelAnimated::RenderUnit* ModelAnimated::addRenderUnit(void) {
        this->m_renderUnits.emplace_back();
        return &this->m_renderUnits.back();
    }

    void ModelAnimated::setSkeletonInterface(SkeletonInterface&& joints) {
        this->m_jointInterface = std::move(joints);
    }

    void ModelAnimated::setAnimations(std::vector<Animation>&& animations) {
        this->m_animations = std::move(animations);
    }

    void ModelAnimated::setGlobalMat(const glm::mat4 mat) {
        this->m_globalInvMat = glm::inverse(mat);
    }

    bool ModelAnimated::isReady(void) const {
        for ( const auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) return false;
        }

        return true;
    }

    void ModelAnimated::render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf,
        const UniInterfAnime& unilocAnime, const glm::mat4 modelMat)
    {
        if ( !this->isReady() ) return;

        this->m_jointInterface.sendUniform(unilocAnime);

        for ( auto& unit : this->m_renderUnits ) {
            unit.m_material.sendUniform(unilocLighted, samplerInterf);
            if ( !unit.m_mesh.isReady() ) continue;

            unilocLighted.modelMat(modelMat);
            unit.m_mesh.draw();
        }
    }

    void ModelAnimated::renderDepthMap(const UniInterfGeometry& unilocGeometry, const UniInterfAnime& unilocAnime, const glm::mat4 modelMat) const {
        if ( !this->isReady() ) return;

        this->m_jointInterface.sendUniform(unilocAnime);

        for ( auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) continue;

            unilocGeometry.modelMat(modelMat);
            unit.m_mesh.draw();
        }
    }

    void ModelAnimated::destroyModel(void) {
        for ( auto& unit : this->m_renderUnits ) {
            unit.m_mesh.destroyData();
        }
    }

    void ModelAnimated::updateAnimation0(void) {
        if ( this->m_animations.empty() ) {
            return;
        }

        const auto& anim = this->m_animations.back();
        const auto elapsed = this->m_animLocalTimer.getElapsed();
        const auto animDuration = anim.getDurationInTick();
        const auto animTickPerSec = anim.getTickPerSec();

        float TimeInTicks = elapsed * animTickPerSec;
        const auto animTick = fmod(TimeInTicks, animDuration);

        anim.sample(animTick, this->m_jointInterface, this->m_globalInvMat);
    }

}


// ModelStaticHandle
namespace dal {

    struct ModelStaticHandleImpl {
        ModelStatic* m_model = nullptr;
        unsigned int m_refCount = 0;

        //static void* operator new(size_t) = delete;
        static void* operator new[](size_t) = delete;
        static void operator delete(void*) = delete;
        static void operator delete[](void*) = delete;
    };

    dal::StaticPool<dal::ModelStaticHandleImpl, 20> g_staticModelCtrlBlckPool;


    ModelStaticHandle::ModelStaticHandle(void)
        : m_pimpl(g_staticModelCtrlBlckPool.alloc())
    {

    }

    ModelStaticHandle::ModelStaticHandle(dal::ModelStatic* const model)
        : m_pimpl(g_staticModelCtrlBlckPool.alloc())
    {
        this->m_pimpl->m_model = model;
    }
    

    ModelStaticHandle::~ModelStaticHandle(void) {
        dalAssert(nullptr != this->m_pimpl);

        --this->m_pimpl->m_refCount;
        if ( 0 == this->m_pimpl->m_refCount ) {
            if ( nullptr != this->m_pimpl->m_model ) {
                g_modelPool.free(this->m_pimpl->m_model);
                this->m_pimpl->m_model = nullptr;
            }

            g_staticModelCtrlBlckPool.free(this->m_pimpl);
            this->m_pimpl = nullptr;
        }
    }

    ModelStaticHandle::ModelStaticHandle(ModelStaticHandle&& other) noexcept
        : m_pimpl(g_staticModelCtrlBlckPool.alloc())
    {
        std::swap(this->m_pimpl, other.m_pimpl);
    }

    ModelStaticHandle::ModelStaticHandle(const ModelStaticHandle& other)
        : m_pimpl(other.m_pimpl)
    {
        ++this->m_pimpl->m_refCount;
    }

    ModelStaticHandle& ModelStaticHandle::operator=(const ModelStaticHandle& other) {
        this->m_pimpl = other.m_pimpl;
        ++this->m_pimpl->m_refCount;
        return *this;
    }

    ModelStaticHandle& ModelStaticHandle::operator=(ModelStaticHandle&& other) noexcept {
        std::swap(this->m_pimpl, other.m_pimpl);
        return *this;
    }

    bool ModelStaticHandle::operator==(ModelStaticHandle& other) const {
        dalAssert(nullptr != this->m_pimpl);
        dalAssert(nullptr != other.m_pimpl);

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

    void ModelStaticHandle::render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf, const glm::mat4& modelMat) const {
        dalAssert(nullptr != this->m_pimpl);
        if ( nullptr != this->m_pimpl->m_model ) {
            this->m_pimpl->m_model->render(unilocLighted, samplerInterf, modelMat);
        }
        else {
            dalAbort("Not implemented.");
        }
    }

    void ModelStaticHandle::renderDepthMap(const UniInterfGeometry& unilocGeometry, const glm::mat4& modelMat) const {
        dalAssert(nullptr != this->m_pimpl);
        if ( nullptr != this->m_pimpl->m_model ) {
            this->m_pimpl->m_model->renderDepthMap(unilocGeometry, modelMat);
        }
        else {
            dalAbort("Not implemented.");
        }
    }

    unsigned int ModelStaticHandle::getRefCount(void) const {
        return this->m_pimpl->m_refCount;
    }

    const AxisAlignedBoundingBox& ModelStaticHandle::getBoundingBox(void) const {
        dalAssert(nullptr != this->m_pimpl);
        if ( nullptr != this->m_pimpl->m_model ) {
            return this->m_pimpl->m_model->getBoundingBox();
        }
        else {
            dalAbort("Not implemented.");
        }
    }

    const ResourceID& ModelStaticHandle::getResID(void) const {
        dalAssert(nullptr != this->m_pimpl);
        if ( nullptr != this->m_pimpl->m_model ) {
            return this->m_pimpl->m_model->getModelResID();
        }
        else {
            dalAbort("Not implemented.");
        }
    }

}


// Package
namespace dal {

    void Package::setName(const char* const packageName) {
        this->m_name = packageName;
    }

    void Package::setName(const std::string& packageName) {
        this->m_name = packageName;
    }

    ModelStaticHandle Package::orderModel(const ResourceID& resPath, ResourceMaster* const resMas) {
        std::string modelIDStr{ resPath.makeFileName() };

        auto iter = this->m_models.find(modelIDStr);
        if ( this->m_models.end() != iter ) {
            return iter->second;
        }
        else {
            auto model = g_modelPool.alloc(); dalAssert(nullptr != model);
            ModelStaticHandle modelHandle{ model };
            model->setModelResID(resPath);
            this->m_models.emplace(modelIDStr, modelHandle);

            ResourceID idWithPackage{ this->m_name, resPath.getOptionalDir(), resPath.getBareName(), resPath.getExt() };
            auto task = new LoadTask_Model{ idWithPackage, *model, *this };
            g_sentTasks_model.insert(task);
            TaskGod::getinst().orderTask(task, resMas);

            return std::move(modelHandle);
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

    ModelStaticHandle Package::buildModel(const loadedinfo::ModelDefined& info, ResourceMaster* const resMas) {
        auto model = g_modelPool.alloc();
        ModelStaticHandle modelHandle{ model };
        this->m_models.emplace(info.m_modelID, modelHandle);

        model->init(info, *resMas);

        return modelHandle;
    }

    Texture* Package::orderDiffuseMap(ResourceID texID, ResourceMaster* const resMas) {
        auto iter = this->m_textures.find(texID.makeFileName());
        if ( this->m_textures.end() == iter ) {
            auto texture = g_texturePool.alloc();
            this->m_textures.emplace(texID.makeFileName(), ManageInfo<Texture>{ texture, 2 });  // ref count is 2 because of return and task.

            if ( texID.getPackage().empty() ) {
                texID.setPackage(this->m_name);
            }
            auto task = new LoadTask_Texture(texID, texture);
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

    void Package::clear(void) {
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

            loaded->data_coresponding.init(loaded->in_modelID, loaded->out_info, *this);
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
            loaded->data_coresponding.setGlobalMat(loaded->out_info.m_model.m_globalTrans);

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

    ModelStaticHandle ResourceMaster::orderModel(const ResourceID& resID) {
        auto& package = this->orderPackage(resID.getPackage());

        return package.orderModel(resID, this);
    }

    ModelAnimated* ResourceMaster::orderModelAnimated(const ResourceID& resID) {
        auto& package = this->orderPackage(resID.getPackage());

        return package.orderModelAnimated(resID, this);
    }

    ModelStaticHandle ResourceMaster::buildModel(const loadedinfo::ModelDefined& info, const char* const packageName) {
        auto& package = this->orderPackage(packageName);
        return package.buildModel(info, this);
    }

    Texture* ResourceMaster::orderTexture(const ResourceID& resID) {
        const auto& packageName = resID.getPackage();
        auto& package = this->orderPackage(packageName);
        return package.orderDiffuseMap(resID, this);
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

        auto iter = this->m_packages.find(packNameStr);
        if ( iter != this->m_packages.end() ) {
            return iter->second;
        }
        else { // If not found
            Package package;
            package.setName(packName);
            auto res = this->m_packages.emplace(packName, std::move(package));
            return res.first->second;
        }
    }

}