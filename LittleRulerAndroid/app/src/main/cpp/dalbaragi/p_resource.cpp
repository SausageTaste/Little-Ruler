#include "p_resource.h"

#include <limits>

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>

#include "s_logger_god.h"
#include "u_objparser.h"
#include "u_pool.h"
#include "s_configs.h"
#include "u_dlbparser.h"


#define BLOCKY_TEXTURE 0


using namespace fmt::literals;


// Tasks
namespace {

    class LoadTaskManger {

    public:
        enum class ResTyp { texture, model_static, model_animated, cube_map };

    public:
        class TaskTexture : public dal::ITask {

        public:
            const dal::ResourceID in_texID;

            dal::binfo::ImageFileData out_img;

            bool out_success = false;

            dal::Texture* data_handle;

        public:
            TaskTexture(const dal::ResourceID& texID, dal::Texture* const handle)
                : in_texID(texID)
                , data_handle(handle)
            {

            }

            virtual void start(void) override {
                out_success = dal::futil::getRes_image(in_texID, out_img);
                this->out_img.m_hasTransparency = this->out_img.hasTransparency();
            }

        };

        class TaskModelStatic : public dal::ITask {

        public:
            const dal::ResourceID in_modelID;

            bool out_success;
            dal::AssimpModelInfo out_info;

            dal::ModelStatic& data_coresponding;
            dal::Package& data_package;

        public:
            TaskModelStatic(const dal::ResourceID& modelID, dal::ModelStatic& coresponding, dal::Package& package)
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

        class TaskModelAnimated : public dal::ITask {

        public:
            const dal::ResourceID in_modelID;

            bool out_success;
            dal::AssimpModelInfo out_info;

            dal::ModelAnimated& data_coresponding;
            dal::Package& data_package;

        public:
            TaskModelAnimated(const dal::ResourceID& modelID, dal::ModelAnimated& coresponding, dal::Package& package)
                : in_modelID(modelID),
                out_success(false),
                data_coresponding(coresponding),
                data_package(package)
            {

            }

            virtual void start(void) override {
                this->out_success = dal::loadAssimpModel(this->in_modelID, this->out_info);
                if ( 0 == this->out_info.m_model.m_joints.getSize() ) {
                    this->out_success = false;
                }
            }

        };

        class TaskCubeMap : public dal::ITask {

        public:
            std::array<dal::ResourceID, 6> in_resIDs;

            bool out_success;
            dal::binfo::ImageFileData out_imgs[6];

            dal::CubeMap* data_handle;

        public:
            TaskCubeMap(const std::array<dal::ResourceID, 6>& resIDs, dal::CubeMap* handle)
                : in_resIDs(resIDs)
                , out_success(false)
                , data_handle(handle)
            {

            }

            virtual void start(void) override {
                this->out_success = true;

                for ( int i = 0; i < 6; ++i ) {
                    const auto result = dal::futil::getRes_image(this->in_resIDs[i], this->out_imgs[i]);
                    if ( !result ) {
                        this->out_success = false;
                        return;
                    }
                    else {
                        this->out_imgs[i].m_hasTransparency = this->out_imgs[i].hasTransparency();

                        switch ( i ) {
                        case 2:
                            this->out_imgs[i].rotate90();
                            break;
                        case 3:
                            this->out_imgs[i].rotate270();
                            break;
                        default:
                            this->out_imgs[i].rotate180();
                            break;
                        }
                    }
                }
            }

        };

    private:
        std::unordered_map<void*, ResTyp> m_map;

    public:
        TaskTexture* newTexture(const dal::ResourceID& texID, dal::Texture* const handle) {
            auto task = new TaskTexture(texID, handle);
            this->m_map.emplace(task, ResTyp::texture);
            return task;
        }
        TaskModelStatic* newModelStatic(const dal::ResourceID& modelID, dal::ModelStatic& coresponding, dal::Package& package) {
            auto task = new TaskModelStatic(modelID, coresponding, package);
            this->m_map.emplace(task, ResTyp::model_static);
            return task;
        }
        TaskModelAnimated* newModelAnimated(const dal::ResourceID& modelID, dal::ModelAnimated& coresponding, dal::Package& package) {
            auto task = new TaskModelAnimated(modelID, coresponding, package);
            this->m_map.emplace(task, ResTyp::model_animated);
            return task;
        }
        TaskCubeMap* newCubeMap(const std::array<dal::ResourceID, 6>& resIDs, dal::CubeMap* const handle) {
            auto task = new TaskCubeMap(resIDs, handle);
            this->m_map.emplace(task, ResTyp::cube_map);
            return task;
        }

        ResTyp reportDone(void* const ptr) {
            const auto found = this->m_map.find(ptr);
            dalAssert(this->m_map.end() != found);
            const auto tasktype = found->second;
            this->m_map.erase(found);
            return tasktype;
        }

    } g_taskManger;

}


namespace {

    const auto ERR_FORMAT_STR = "Trying to add a {} which already exists: package{{ {} }}, res id{{ {} }}";

}


// Map chunk 2
namespace dal {

    void MapChunk2::onWinResize(const unsigned int winWidth, const unsigned int winHeight) {
        for ( auto& water : this->m_waters ) {
            water.onWinResize(winWidth, winHeight);
        }
    }


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
        this->sendLightUniforms(uniloc.m_lightedMesh, 0);

        for ( const auto& [model, actors] : this->m_staticActors ) {
            for ( const auto& actor : actors ) {
                model.render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), actor.m_transform.getMat());
            }
        }
    }

    void MapChunk2::renderDepthGeneral(const UnilocDepthmp& uniloc) {
        for ( const auto& [mdl, actors] : this->m_staticActors ) {
            for ( const auto& actor : actors ) {
                mdl.renderDepth(uniloc.m_geometry, actor.m_transform.getMat());
            }
        }
    }

    void MapChunk2::renderWater(const UnilocWaterry& uniloc) {
        this->sendLightUniforms(uniloc.m_lightedMesh, 0);

        for ( auto& water : this->m_waters ) {
            water.render(uniloc);
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
                        model.m_model.render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), trans.getMat());
                    }
                );
            }

            {
                water.startRenderOnRefrac(uniloc, cam);
                glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
                this->renderGeneral(uniloc);

                view.each(
                    [&uniloc](const cpnt::Transform& trans, const cpnt::StaticModel& model) {
                        model.m_model.render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), trans.getMat());
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

                    cpntModel.m_model.render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), uniloc.m_anime, cpntTransform.getMat(),
                        cpntModel.m_animState.getTransformArray());
                }
            }

            {
                water.startRenderOnRefrac(uniloc, cam);
                //this->renderAnimate(uniloc);

                for ( const auto entity : view ) {
                    auto& cpntTransform = view.get<cpnt::Transform>(entity);
                    auto& cpntModel = view.get<cpnt::AnimatedModel>(entity);

                    cpntModel.m_model.render(uniloc.m_lightedMesh, uniloc.getDiffuseMapLoc(), uniloc.m_anime, cpntTransform.getMat(),
                        cpntModel.m_animState.getTransformArray());
                }
            }
        }
    }

    void MapChunk2::renderOnWaterSkybox(const UnilocSkybox& uniloc, const Skybox& skybox, const ICamera& cam) {
        for ( auto& water : this->m_waters ) {
            water.startRenderOnReflec(uniloc.m_geometry, cam);
            skybox.render(uniloc);
        }
    }

    // Private

    int MapChunk2::sendLightUniforms(const UniInterfLightedMesh& uniloc, int startIndex) const {
        if ( startIndex >= 3 )
            dalAbort("Too many point lights.");
        if ( startIndex + this->m_plights.size() > 3 )
            dalAbort("Too many point lights.");

        uniloc.plightCount(startIndex + this->m_plights.size());
        for ( size_t i = 0; i < this->m_plights.size(); i++ ) {
            if ( i >= 3 ) break;
            this->m_plights.at(i).sendUniform(uniloc, startIndex + i);
        }

        return startIndex + this->m_plights.size();
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

    std::optional<ModelAnimatedHandle> Package::getModelAnim(const ResourceID& resID) {
        auto found = this->m_animatedModels.find(resID.makeFileName());
        if ( this->m_animatedModels.end() == found ) {
            return std::nullopt;
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

    bool Package::giveModelStatic(const ResourceID& resID, const ModelStaticHandle& mdl) {
        const auto filename = resID.makeFileName();

        if ( this->m_models.end() != this->m_models.find(filename) ) {
            dalError(fmt::format(ERR_FORMAT_STR, "static model", this->m_name, resID.makeIDStr()));
            return false;
        }
        else {
            this->m_models.emplace(filename, mdl);
            return true;
        }
    }

    bool Package::giveModelAnim(const ResourceID& resID, const ModelAnimatedHandle& mdl) {
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
        dalAssert(nullptr != task);

        const auto taskTyp = g_taskManger.reportDone(task.get());

        if ( taskTyp == LoadTaskManger::ResTyp::model_static ) {
            auto loaded = reinterpret_cast<LoadTaskManger::TaskModelStatic*>(task.get());
            if ( !loaded->out_success ) {
                dalError("Failed to load model: {}"_format(loaded->in_modelID.makeIDStr()));
                return;
            }

            //loaded->data_coresponding.init(loaded->in_modelID, loaded->out_info, *this);
            {
                loaded->data_coresponding.invalidate();

                loaded->data_coresponding.m_resID = std::move(loaded->in_modelID);

                for ( auto& unitInfo : loaded->out_info.m_model.m_renderUnits ) {
                    auto& unit = loaded->data_coresponding.newRenderUnit();

                    unit.m_mesh.buildData(
                        unitInfo.m_mesh.m_vertices.data(),
                        unitInfo.m_mesh.m_texcoords.data(),
                        unitInfo.m_mesh.m_normals.data(),
                        unitInfo.m_mesh.m_vertices.size() / 3
                    );
                    unit.m_meshName = unitInfo.m_name;

                    unit.m_material.m_diffuseColor = unitInfo.m_material.m_baseColor;
                    unit.m_material.m_shininess = unitInfo.m_material.m_shininess;
                    unit.m_material.m_specularStrength = unitInfo.m_material.m_specStreng;
                    unit.m_material.m_reflectivity = 0.f;

                    if ( !unitInfo.m_material.m_diffuseMap.empty() ) {
                        ResourceID texResID{ unitInfo.m_material.m_diffuseMap };
                        texResID.setPackageIfEmpty(loaded->in_modelID.getPackage());
                        auto tex = this->orderTexture(texResID);
                        unit.m_material.setDiffuseMap(tex);
                    }
                }

                loaded->data_coresponding.m_bounding.reset(new ColAABB{ loaded->out_info.m_model.m_aabb });
            }
        }
        else if ( taskTyp == LoadTaskManger::ResTyp::texture ) {
            auto loaded = reinterpret_cast<LoadTaskManger::TaskTexture*>(task.get());
            if ( !loaded->out_success ) {
                dalError("Failed to load texture: {}"_format(loaded->in_texID.makeIDStr()));
                return;
            }

            loaded->data_handle->init_diffuseMap(loaded->out_img);
        }
        else if ( taskTyp == LoadTaskManger::ResTyp::model_animated ) {
            auto loaded = reinterpret_cast<LoadTaskManger::TaskModelAnimated*>(task.get());
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

                unit->m_material.m_diffuseColor = unitInfo.m_material.m_baseColor;
                unit->m_material.m_shininess = unitInfo.m_material.m_shininess;
                unit->m_material.m_specularStrength = unitInfo.m_material.m_specStreng;

                if ( !unitInfo.m_material.m_diffuseMap.empty() ) {
                    ResourceID diffuseResID{ unitInfo.m_material.m_diffuseMap };
                    diffuseResID.setPackageIfEmpty(loaded->in_modelID.getPackage());

                    auto tex = this->orderTexture(diffuseResID);
                    unit->m_material.setDiffuseMap(tex);
                }
            }
        }
        else if ( taskTyp == LoadTaskManger::ResTyp::cube_map ) {
            auto loaded = reinterpret_cast<LoadTaskManger::TaskCubeMap*>(task.get());
            if ( !loaded->out_success ) {
                dalError("Failed to load cube map: {}"_format(loaded->in_resIDs[0].makeIDStr()));
                return;
            }

            CubeMap::CubeMapData data;

            for ( int i = 0; i < 6; ++i ) {
                auto& info = loaded->out_imgs[i];
                data.set(i, info.m_buf.data(), info.m_width, info.m_height, info.m_pixSize);
            }

            loaded->data_handle->init(data);
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

            auto task = g_taskManger.newModelStatic(resID, *model, package);
            TaskGod::getinst().orderTask(task, this);

            return modelHandle;
        }
    }

    ModelAnimatedHandle ResourceMaster::orderModelAnim(const ResourceID& resID) {
        auto& package = this->orderPackage(resID.getPackage());

        auto found = package.getModelAnim(resID);
        if ( found ) {
            return *found;
        }
        else {
            auto model = new ModelAnimated; dalAssert(nullptr != model);
            ModelAnimatedHandle modelHandle{ model };
            model->m_resID = resID;
            package.giveModelAnim(resID, modelHandle);

            auto task = g_taskManger.newModelAnimated(resID, *model, package);
            TaskGod::getinst().orderTask(task, this);

            return modelHandle;
        }
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

            auto task = g_taskManger.newTexture(resID, texture);
            TaskGod::getinst().orderTask(task, this);

            return texture;
        }
    }

    CubeMap* ResourceMaster::orderCubeMap(const std::array<ResourceID, 6>& resIDs) {
        auto tex = &this->m_cubeMaps.emplace_back();

        auto task = g_taskManger.newCubeMap(resIDs, tex);
        TaskGod::getinst().orderTask(task, this);

        return tex;
    }


    MapChunk2 ResourceMaster::loadMap(const ResourceID& resID) {
        std::vector<uint8_t> buffer;
        futil::getRes_buffer(resID, buffer);
        auto mapInfo = parseDLB(buffer.data(), buffer.size());
        if ( !mapInfo ) {
            dalAbort("Failed to load map : {}"_format(resID.makeIDStr()));
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
                    unitInfo.m_mesh.m_vertices.data(),
                    unitInfo.m_mesh.m_texcoords.data(),
                    unitInfo.m_mesh.m_normals.data(),
                    unitInfo.m_mesh.m_vertices.size() / 3
                );

                unit.m_material.m_diffuseColor = unitInfo.m_material.m_baseColor;
                unit.m_material.m_shininess = unitInfo.m_material.m_shininess;
                unit.m_material.m_specularStrength = unitInfo.m_material.m_specStreng;
                unit.m_material.setTexScale(unitInfo.m_material.m_texScale);
                unit.m_material.m_reflectivity = unitInfo.m_material.m_reflectivity;

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

        for ( auto& light : mapInfo->m_plights ) {
            auto& mapLight = map.newPlight();
            mapLight.mPos = light.m_pos;
            mapLight.m_color = light.m_color;
            mapLight.mMaxDistance = light.m_maxDist;
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
