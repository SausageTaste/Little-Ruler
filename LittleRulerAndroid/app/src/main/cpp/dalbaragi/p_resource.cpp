#include "p_resource.h"

#include <limits>

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/matrix4x4.h>

#include "s_logger_god.h"
#include "u_objparser.h"
#include "u_pool.h"
#include "s_configs.h"


#define BLOCKY_TEXTURE 0


using namespace fmt::literals;


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


namespace {

    const auto ERR_FORMAT_STR = "Trying to add a {} which already exists: package{{ {} }}, res id{{ {} }}";

}


// Map chunk 2
namespace dal {

    void MapChunk2::addWaterPlane(const dlb::WaterPlane& waterInfo) {
        const auto width = GlobalStateGod::getinst().getWinWidth();
        const auto height = GlobalStateGod::getinst().getWinHeight();

        this->m_waters.emplace_back(waterInfo, width, height);
    }


    void MapChunk2::applyCollision(const ICollider& inCol, cpnt::Transform& inTrans) {
        for ( auto& mdl : this->m_staticActors ) {
            const auto mdlBounding = mdl.m_model.getBounding();
            if ( nullptr == mdlBounding ) {
                continue;
            }

            for ( auto& actor : mdl.m_actors ) {
                const auto withBounding = checkCollisionAbs(inCol, *mdlBounding, inTrans, actor.m_transform);
                if ( !withBounding ) {
                    continue;
                }

                const auto detailedCol = mdl.m_model.getDetailed();
                if ( nullptr != detailedCol ) {
                    const auto withDetailed = checkCollisionAbs(inCol, *detailedCol, inTrans, actor.m_transform);
                    if ( withDetailed ) {

                    }
                }
            }
        }
    }

    std::optional<RayCastingResult> MapChunk2::castRayToClosest(const Ray& ray) const {
        float closestDistance = std::numeric_limits<float>::max();
        std::optional<RayCastingResult> result{ std::nullopt };

        for ( auto& modelActor : this->m_staticActors ) {
            const auto mdlBounding = modelActor.m_model.getBounding();
            if ( nullptr == mdlBounding ) {
                continue;
            }

            const auto mdlDetailed = modelActor.m_model.getDetailed();

            if ( nullptr != mdlDetailed ) {
                for ( auto& actor : modelActor.m_actors ) {
                    if ( !checkCollisionAbs(ray, *mdlBounding, actor.m_transform) ) {
                        continue;
                    }
                    const auto res = calcCollisionInfoAbs(ray, *mdlDetailed, actor.m_transform);
                    if ( !res ) {
                        continue;
                    }
                    if ( res->m_distance < closestDistance ) {
                        closestDistance = res->m_distance;
                        result = *res;
                    }
                }
            }
            else {
                for ( auto& actor : modelActor.m_actors ) {
                    const auto res = calcCollisionInfoAbs(ray, *mdlBounding, actor.m_transform);
                    if ( !res ) {
                        continue;
                    }
                    if ( res->m_distance < closestDistance ) {
                        closestDistance = res->m_distance;
                        result = *res;
                    }
                }
            }
        }

        return result;
    }


    void MapChunk2::renderGeneral(const UnilocGeneral& uniloc) {
        for ( const auto& [model, actors] : this->m_staticActors ) {
            for ( const auto& actor : actors ) {
                model.render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), actor.m_transform.getMat());
            }
        }
    }

    void MapChunk2::renderDepthMp(const UnilocDepthmp& uniloc) {
        for ( const auto& [mdl, actors] : this->m_staticActors ) {
            for ( const auto& actor : actors ) {
                mdl.renderDepthMap(uniloc.m_geometry, actor.m_transform.getMat());
            }
        }
    }

    void MapChunk2::renderWater(const UnilocWaterry& uniloc) {
        for ( auto& water : this->m_waters ) {
            water.renderWaterry(uniloc);
        }
    }

    void MapChunk2::renderOnWaterGeneral(const UnilocGeneral& uniloc, const ICamera& cam, entt::registry& reg) {
        const auto view = reg.view<cpnt::Transform, cpnt::StaticModel>();

        for ( auto& water : this->m_waters ) {
            {
                water.startRenderOnReflec(uniloc, cam);
                glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
                this->renderGeneral(uniloc);

                view.each(
                    [&uniloc](const cpnt::Transform& trans, const cpnt::StaticModel& model) {
                        model.m_model->render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), trans.getMat());
                    }
                );
            }

            {
                water.startRenderOnRefrac(uniloc, cam);
                glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
                this->renderGeneral(uniloc);

                view.each(
                    [&uniloc](const cpnt::Transform& trans, const cpnt::StaticModel& model) {
                        model.m_model->render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), trans.getMat());
                    }
                );
            }
        }
    }

    void MapChunk2::renderOnWaterAnimated(const UnilocAnimate& uniloc, const ICamera& cam, entt::registry& reg) {
        const auto view = reg.view<cpnt::Transform, cpnt::AnimatedModel>();

        for ( auto& water : this->m_waters ) {
            {
                water.startRenderOnReflec(uniloc, cam);
                //this->renderAnimate(uniloc);

                for ( const auto entity : view ) {
                    auto& cpntTransform = view.get<cpnt::Transform>(entity);
                    auto& cpntModel = view.get<cpnt::AnimatedModel>(entity);

                    cpntModel.m_model->render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), uniloc.m_anime, cpntTransform.getMat(),
                        cpntModel.m_animState.getTransformArray());
                }
            }

            {
                water.startRenderOnRefrac(uniloc, cam);
                //this->renderAnimate(uniloc);

                for ( const auto entity : view ) {
                    auto& cpntTransform = view.get<cpnt::Transform>(entity);
                    auto& cpntModel = view.get<cpnt::AnimatedModel>(entity);

                    cpntModel.m_model->render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), uniloc.m_anime, cpntTransform.getMat(),
                        cpntModel.m_animState.getTransformArray());
                }
            }
        }
    }

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

    Package::Package(Package&& other) noexcept
        : m_name(std::move(other.m_name))
        , m_models(std::move(other.m_models))
        , m_animatedModels(std::move(other.m_animatedModels))
        , m_textures(std::move(other.m_textures))
    {

    }

    Package& Package::operator=(Package&& other) noexcept {
        this->m_name = std::move(other.m_name);
        this->m_models = std::move(other.m_models);
        this->m_animatedModels = std::move(other.m_animatedModels);
        this->m_textures = std::move(other.m_textures);

        return *this;
    }

    Package::~Package(void) {
        for ( auto& [id, pMdl] : this->m_animatedModels ) {
            delete pMdl;
        }
        for ( auto& [id, pTex] : this->m_textures ) {
            delete pTex;
        }
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

            //loaded->data_coresponding.init(loaded->in_modelID, loaded->out_info, *this);
            {
                loaded->data_coresponding.invalidate();

                loaded->data_coresponding.m_resID = std::move(loaded->in_modelID);

                for ( auto& unitInfo : loaded->out_info.m_renderUnits ) {
                    auto& unit = loaded->data_coresponding.newRenderUnit();

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
                        texResID.setPackageIfEmpty(loaded->in_modelID.getPackage());
                        auto tex = this->orderTexture(texResID);
                        unit.m_material.setDiffuseMap(tex);
                    }
                }

                loaded->data_coresponding.m_bounding.reset(new ColAABB{ loaded->out_info.m_aabb });
            }
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
            auto model = new ModelStatic; dalAssert(nullptr != model);
            ModelStaticHandle modelHandle{ model };
            model->m_resID = resID;  // It might not be resolved.
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
            auto model = new ModelAnimated;
            model->m_resID = resID;
            package.giveModelAnim(resID, model);

            auto task = new LoadTask_ModelAnimated{ resID, *model, package };
            g_sentTasks_modelAnimated.insert(task);
            TaskGod::getinst().orderTask(task, this);

            return model;
        }
    }

    ModelStaticHandle ResourceMaster::buildModel(const loadedinfo::ModelDefined& info, const std::string& packageName) {
        auto& package = this->orderPackage(packageName);

        auto model = new ModelStatic;

        ResourceID resID{ info.m_modelID };
        resID.setPackage(packageName);
        model->m_resID = resID;

        ModelStaticHandle modelHandle{ model };
        package.giveModelStatic(info.m_modelID, modelHandle);

        {
            model->invalidate();

            auto& unitInfo = info.m_renderUnit;
            auto& unit = model->newRenderUnit();

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
                auto tex = this->orderTexture(unitInfo.m_material.m_diffuseMap);
                unit.m_material.setDiffuseMap(tex);
            }
        }

        return modelHandle;
    }

    Texture* ResourceMaster::orderTexture(const ResourceID& resID) {
        auto& package = this->orderPackage(resID.getPackage());

        auto found = package.getTexture(resID);
        if ( nullptr != found ) {
            return found;
        }
        else {
            auto texture = new Texture;
            package.giveTexture(resID, texture);

            auto task = new LoadTask_Texture(resID, texture);
            g_sentTasks_texture.insert(task);
            TaskGod::getinst().orderTask(task, this);

            return texture;
        }
    }

    MapChunk2 ResourceMaster::loadMap(const ResourceID& resID) {
        std::vector<uint8_t> buffer;
        futil::getRes_buffer(resID, buffer);
        auto mapInfo = parseDLB(buffer.data(), buffer.size());
        if ( !mapInfo ) {
            dalAbort("Shit!");
        }

        MapChunk2 map;

        for ( auto& mdlEmbed : mapInfo->m_embeddedModels ) {
            auto model = new ModelStatic;
            ModelStaticHandle modelHandle{ model };

            // Name
            {
                ResourceID mdlResName{ mdlEmbed.m_name };
                mdlResName.setPackage(resID.getPackage());
                model->m_resID = mdlResName;
            }

            // Render units
            for ( const auto& unitInfo : mdlEmbed.m_renderUnits ) {
                auto& unit = model->newRenderUnit();
                unit.m_mesh.buildData(
                    unitInfo.m_mesh.m_vertices.data(), unitInfo.m_mesh.m_vertices.size(),
                    unitInfo.m_mesh.m_texcoords.data(), unitInfo.m_mesh.m_texcoords.size(),
                    unitInfo.m_mesh.m_normals.data(), unitInfo.m_mesh.m_normals.size()
                );

                unit.m_material.m_diffuseColor = unitInfo.m_material.m_baseColor;
                unit.m_material.m_shininess = unitInfo.m_material.m_shininess;
                unit.m_material.m_specularStrength = unitInfo.m_material.m_specStreng;
                unit.m_material.setTexScale(unitInfo.m_material.m_texScale);

                if ( !unitInfo.m_material.m_diffuseMap.empty() ) {
                    auto tex = this->orderTexture(unitInfo.m_material.m_diffuseMap);
                    unit.m_material.setDiffuseMap(tex);
                }
            }

            // Colliders
            {
                model->m_bounding = std::move(mdlEmbed.m_bounding);
                model->m_detailed = std::move(mdlEmbed.m_detailed);
            }

            map.addStaticActorModel(std::move(modelHandle), std::move(mdlEmbed.m_staticActors));
        }

        for ( auto& mdlImport : mapInfo->m_importedModels ) {
            auto model = this->orderModelStatic(mdlImport.m_resourceID);
            map.addStaticActorModel(std::move(model), std::move(mdlImport.m_staticActors));
        }

        for ( auto& water : mapInfo->m_waterPlanes ) {
            map.addWaterPlane(water);
        }

        return map;
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