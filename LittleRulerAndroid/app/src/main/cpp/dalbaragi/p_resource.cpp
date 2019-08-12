#include "p_resource.h"

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/matrix4x4.h>

#include "s_logger_god.h"
#include "u_objparser.h"
#include "u_pool.h"


#define BLOCKY_TEXTURE 0


using namespace fmt::literals;


namespace {

    inline glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4* from) {
        glm::mat4 to;

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
        const UniInterfAnime& unilocAnime, const glm::mat4 modelMat, const JointTransformArray& transformArr)
    {
        if ( !this->isReady() ) return;

        transformArr.sendUniform(unilocAnime);

        for ( auto& unit : this->m_renderUnits ) {
            unit.m_material.sendUniform(unilocLighted, samplerInterf);
            if ( !unit.m_mesh.isReady() ) continue;

            unilocLighted.modelMat(modelMat);
            unit.m_mesh.draw();
        }
    }

    void ModelAnimated::renderDepthMap(const UniInterfGeometry& unilocGeometry, const UniInterfAnime& unilocAnime, const glm::mat4 modelMat,
        const JointTransformArray& transformArr) const {
        if ( !this->isReady() ) return;

        transformArr.sendUniform(unilocAnime);

        for ( auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) continue;

            unilocGeometry.modelMat(modelMat);
            unit.m_mesh.draw();
        }
    }

    void ModelAnimated::destroyModel(void) {
        this->m_renderUnits.clear();
    }

    /*
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
    */

}


// ModelStaticHandle
namespace dal {

    struct ModelStaticHandleImpl {
        ModelStatic* m_model = nullptr;
        unsigned int m_refCount = 1;
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
        if ( nullptr == this->m_pimpl ) {
            return;
        }

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
        : m_pimpl(nullptr)
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


namespace {

    const auto ERR_FORMAT_STR = "Trying to add a {} which already exists: package{{ {} }}, res id{{ {} }}";

}


// Package
namespace dal {

    Package::Package(const std::string& pckName)
        : m_name(pckName)
    {

    }

    Package::Package(std::string&& pckName)
        : m_name(std::move(pckName))
    {

    }


    bool Package::hasTexture(const ResourceID& resID) {
        return this->m_textures.end() != this->m_textures.find(resID.makeFileName());
    }

    bool Package::hasModelStatic(const ResourceID& resID) {
        return this->m_models.end() != this->m_models.find(resID.makeFileName());
    }

    bool Package::hasModelAnim(const ResourceID& resID) {
        return this->m_animatedModels.end() != this->m_animatedModels.find(resID.makeFileName());
    }

    std::optional<ModelStaticHandle> Package::getModelStatic(const ResourceID& resID) {
        auto found = this->m_models.find(resID.makeFileName());
        if ( this->m_models.end() == found ) {
            return std::nullopt;
        }
        else {
            return found->second;
        }
    }

    ModelAnimated* Package::getModelAnim(const ResourceID& resID) {
        auto found = this->m_animatedModels.find(resID.makeFileName());
        if ( this->m_animatedModels.end() == found ) {
            return nullptr;
        }
        else {
            return found->second;
        }
    }

    Texture* Package::getTexture(const ResourceID& resID) {
        auto found = this->m_textures.find(resID.makeFileName());
        if ( this->m_textures.end() == found ) {
            return nullptr;
        }
        else {
            return found->second;
        }
    }

    bool Package::giveModelStatic(const ResourceID& resID, ModelStaticHandle mdl) {
        const auto filename = resID.makeFileName();

        if ( this->m_models.end() != this->m_models.find(filename) ) {
            dalError(fmt::format(ERR_FORMAT_STR, "static model", this->m_name, resID.makeIDStr()));
            return false;
        }
        else {
            this->m_models.emplace(filename, std::move(mdl));
            return true;
        }
    }

    bool Package::giveModelAnim(const ResourceID& resID, ModelAnimated* const mdl) {
        const auto filename = resID.makeFileName();

        if ( this->m_animatedModels.end() != this->m_animatedModels.find(filename) ) {
            dalError(fmt::format(ERR_FORMAT_STR, "animated model", this->m_name, resID.makeIDStr()));
            return false;
        }
        else {
            this->m_animatedModels.emplace(filename, mdl);
            return true;
        }
    }

    bool Package::giveTexture(const ResourceID& resID, Texture* const tex) {
        const auto filename = resID.makeFileName();

        if ( this->m_textures.end() != this->m_textures.find(filename) ) {
            dalError(fmt::format(ERR_FORMAT_STR, "texture", this->m_name, resID.makeIDStr()));
            return false;
        }
        else {
            this->m_textures.emplace(filename, tex);
            return true;
        }
    }

}


// ResourceMaster
namespace dal {

    void ResourceMaster::notifyTask(std::unique_ptr<ITask> task) {
        if ( nullptr == task ) {
            dalAbort("ResourceMaster::notifyTask has got a nullptr. Why??");
        }

        if ( g_sentTasks_model.find(task.get()) != g_sentTasks_model.end() ) {
            g_sentTasks_model.erase(task.get());

            auto loaded = reinterpret_cast<LoadTask_Model*>(task.get());
            if ( !loaded->out_success ) {
                dalError("Failed to load model: {}"_format(loaded->in_modelID.makeIDStr()));
                return;
            }

            loaded->data_coresponding.init(loaded->in_modelID, loaded->out_info, *this);
        }
        else if ( g_sentTasks_texture.find(task.get()) != g_sentTasks_texture.end() ) {
            g_sentTasks_texture.erase(task.get());

            auto loaded = reinterpret_cast<LoadTask_Texture*>(task.get());
            if ( !loaded->out_success ) {
                dalError("Failed to load texture: {}"_format(loaded->in_texID.makeIDStr()));
                return;
            }

            if ( loaded->out_img.m_pixSize == 3 ) {
                loaded->data_handle->init_diffueMap3(loaded->out_img.m_buf.data(), loaded->out_img.m_width, loaded->out_img.m_height);
            }
            else if ( loaded->out_img.m_pixSize == 4 ) {
                loaded->data_handle->init_diffueMap(loaded->out_img.m_buf.data(), loaded->out_img.m_width, loaded->out_img.m_height);
            }
            else {
                dalAbort("Not supported pixel size: {}"_format(loaded->out_img.m_pixSize));
            }
        }
        else if ( g_sentTasks_modelAnimated.find(task.get()) != g_sentTasks_modelAnimated.end() ) {
            g_sentTasks_modelAnimated.erase(task.get());

            auto loaded = reinterpret_cast<LoadTask_ModelAnimated*>(task.get());
            if ( !loaded->out_success ) {
                dalError("Failed to load model: {}"_format(loaded->in_modelID.makeIDStr()));
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
                    ResourceID diffuseResID{ unitInfo.m_material.m_diffuseMap };
                    diffuseResID.setPackageIfEmpty(loaded->in_modelID.getPackage());

                    auto tex = this->orderTexture(diffuseResID);
                    unit->m_material.setDiffuseMap(tex);
                }
            }
        }
        else {
            dalWarn("ResourceMaster got a task that it doesn't know.");
        }
    }


    ModelStaticHandle ResourceMaster::orderModelStatic(const ResourceID& resID) {
        auto& package = this->orderPackage(resID.getPackage());

        auto found = package.getModelStatic(resID);
        if ( found ) {
            return *found;
        }
        else {
            auto model = g_modelPool.alloc(); dalAssert(nullptr != model);
            ModelStaticHandle modelHandle{ model };
            model->setModelResID(resID);  // It might not be resolved.
            package.giveModelStatic(resID, modelHandle);

            auto task = new LoadTask_Model{ resID, *model, package };
            g_sentTasks_model.insert(task);
            TaskGod::getinst().orderTask(task, this);

            return modelHandle;
        }
    }

    ModelAnimated* ResourceMaster::orderModelAnim(const ResourceID& resID) {
        auto& package = this->orderPackage(resID.getPackage());

        auto found = package.getModelAnim(resID);
        if ( nullptr != found ) {
            return found;
        }
        else {
            auto model = g_animatedModelPool.alloc();
            model->setModelResID(resID);
            package.giveModelAnim(resID, model);

            auto task = new LoadTask_ModelAnimated{ resID, *model, package };
            g_sentTasks_modelAnimated.insert(task);
            TaskGod::getinst().orderTask(task, this);

            return model;
        }
    }

    ModelStaticHandle ResourceMaster::buildModel(const loadedinfo::ModelDefined& info, const std::string& packageName) {
        auto& package = this->orderPackage(packageName);

        auto model = g_modelPool.alloc();
        ModelStaticHandle modelHandle{ model };
        package.giveModelStatic(info.m_modelID, modelHandle);

        model->init(info, *this);

        return modelHandle;
    }

    Texture* ResourceMaster::orderTexture(const ResourceID& resID) {
        auto& package = this->orderPackage(resID.getPackage());

        auto found = package.getTexture(resID);
        if ( nullptr != found ) {
            return found;
        }
        else {
            auto texture = g_texturePool.alloc();
            package.giveTexture(resID, texture);

            auto task = new LoadTask_Texture(resID, texture);
            g_sentTasks_texture.insert(task);
            TaskGod::getinst().orderTask(task, this);

            return texture;
        }
    }

    // Private

    Package& ResourceMaster::orderPackage(const std::string& packName) {
        std::string packNameStr{ packName };

        auto iter = this->m_packages.find(packNameStr);
        if ( iter != this->m_packages.end() ) {
            return iter->second;
        }
        else { // If not found
            auto res = this->m_packages.emplace(packName, packName);
            return res.first->second;
        }
    }

}