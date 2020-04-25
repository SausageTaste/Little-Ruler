#include "p_resource.h"

#include <limits>

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>

#include <d_logger.h>
#include <d_pool.h>
#include <d_filesystem.h>
#include <d_mapparser.h>

#include "u_objparser.h"
#include "s_configs.h"
#include "u_dlbparser.h"
#include "u_fileutils.h"


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
            const std::string in_texID;
            bool in_gammaCorrect;

            dal::ImageData out_img;

            bool out_success = false;

            dal::Texture* data_handle;

        public:
            TaskTexture(const std::string& texID, dal::Texture* const handle, const bool gammaCorrect)
                : in_texID(texID)
                , in_gammaCorrect(gammaCorrect)
                , data_handle(handle)
            {

            }

            virtual void start(void) override {
                this->out_success = dal::loadFileImage(this->in_texID.c_str(), this->out_img);

                if ( this->out_success ) {
                    if ( this->in_gammaCorrect ) {
                        this->out_img.correctSRGB();
                    }
                }
            }

        };

        class TaskModelStatic : public dal::ITask {

        public:
            const std::string in_modelID;

            bool out_success;
            dal::ModelLoadInfo out_info;

            dal::ModelStatic& data_coresponding;
            dal::Package& data_package;

        public:
            TaskModelStatic(const std::string& modelID, dal::ModelStatic& coresponding, dal::Package& package)
                : in_modelID(modelID),
                out_success(false),
                data_coresponding(coresponding),
                data_package(package)
            {

            }

            virtual void start(void) override {
                this->out_success = dal::loadDalModel(this->in_modelID.c_str(), this->out_info);
            }

        };

        class TaskModelAnimated : public dal::ITask {

        public:
            const std::string in_modelID;

            bool out_success;
            dal::ModelLoadInfo out_info;

            dal::ModelAnimated& data_coresponding;
            dal::Package& data_package;

        public:
            TaskModelAnimated(const std::string& modelID, dal::ModelAnimated& coresponding, dal::Package& package)
                : in_modelID(modelID),
                out_success(false),
                data_coresponding(coresponding),
                data_package(package)
            {

            }

            virtual void start(void) override {
                this->out_success = dal::loadDalModel(this->in_modelID.c_str(), this->out_info);

                if ( 0 == this->out_info.m_model.m_joints.getSize() ) {
                    this->out_success = false;
                }
            }

        };

        class TaskCubeMap : public dal::ITask {

        public:
            std::array<std::string, 6> in_resIDs;
            bool in_gammaCorrect;

            bool out_success;
            dal::ImageData out_imgs[6];

            dal::CubeMap* data_handle;

        public:
            TaskCubeMap(const std::array<std::string, 6>& resIDs, dal::CubeMap* handle, const bool gammaCorrect)
                : in_resIDs(resIDs)
                , in_gammaCorrect(gammaCorrect)
                , out_success(false)
                , data_handle(handle)
            {

            }

            virtual void start(void) override {
                this->out_success = true;

                for ( int i = 0; i < 6; ++i ) {
                    const auto result = dal::loadFileImage(this->in_resIDs[i].c_str(), this->out_imgs[i]);
                    if ( !result ) {
                        this->out_success = false;
                        return;
                    }
                    else {
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

                        if ( this->in_gammaCorrect ) {
                            this->out_imgs[i].correctSRGB();
                        }
                    }
                }
            }

        };

    private:
        std::unordered_map<void*, ResTyp> m_map;

    public:
        std::unique_ptr<dal::ITask> newTexture(const std::string& texID, dal::Texture* const handle, const bool gammaCorrect) {
            std::unique_ptr<dal::ITask> task{ new TaskTexture{texID, handle, gammaCorrect} };
            this->m_map.emplace(task.get(), ResTyp::texture);
            return std::move(task);
        }
        std::unique_ptr<dal::ITask> newModelStatic(const std::string& modelID, dal::ModelStatic& coresponding, dal::Package& package) {
            std::unique_ptr<dal::ITask> task{ new TaskModelStatic(modelID, coresponding, package) };
            this->m_map.emplace(task.get(), ResTyp::model_static);
            return std::move(task);
        }
        std::unique_ptr<dal::ITask> newModelAnimated(const std::string& modelID, dal::ModelAnimated& coresponding, dal::Package& package) {
            std::unique_ptr<dal::ITask> task{ new TaskModelAnimated(modelID, coresponding, package) };
            this->m_map.emplace(task.get(), ResTyp::model_animated);
            return std::move(task);
        }
        std::unique_ptr<dal::ITask> newCubeMap(const std::array<std::string, 6>& resIDs, dal::CubeMap* const handle, const bool gammaCorrect) {
            std::unique_ptr<dal::ITask> task{ new TaskCubeMap(resIDs, handle, gammaCorrect) };
            this->m_map.emplace(task.get(), ResTyp::cube_map);
            return std::move(task);
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


// Utils
namespace {

    void copyMaterial(dal::Material& dst, const dal::binfo::Material& src, dal::ResourceMaster& resMas, const std::string& packageName) {
        dst.m_roughness = src.m_roughness;
        dst.m_metallic = src.m_metallic;
        dst.m_texScale = src.m_texScale;

        if ( !src.m_diffuseMap.empty() ) {
            if ( ':' == src.m_diffuseMap.front() ) {
                dst.m_diffuseMap = resMas.orderTexture((packageName + src.m_diffuseMap).c_str(), true);
            }
            else {
                dst.m_diffuseMap = resMas.orderTexture(src.m_diffuseMap.c_str(), true);
            }
        }
        if ( !src.m_roughnessMap.empty() ) {
            if ( ':' == src.m_roughnessMap.front() ) {
                dst.m_roughnessMap = resMas.orderTexture((packageName + src.m_roughnessMap).c_str(), false);
            }
            else {
                dst.m_roughnessMap = resMas.orderTexture(src.m_roughnessMap.c_str(), false);
            }
        }
        if ( !src.m_metallicMap.empty() ) {
            if ( ':' == src.m_metallicMap.front() ) {
                dst.m_metallicMap = resMas.orderTexture((packageName + src.m_metallicMap).c_str(), false);
            }
            else {
                dst.m_metallicMap = resMas.orderTexture(src.m_metallicMap.c_str(), false);
            }
        }

#if DAL_NORMAL_MAPPING
        if ( !src.m_normalMap.empty() ) {
            if ( ':' == src.m_normalMap.front() ) {
                dst.m_normalMap = resMas.orderTexture((packageName + src.m_normalMap).c_str(), false);
            }
            else {
                dst.m_normalMap = resMas.orderTexture(src.m_normalMap.c_str(), false);
            }
        }
#endif
    }

    void copyMaterial(dal::Material& dst, const dal::v1::Material& src, dal::ResourceMaster& resMas, const std::string& packageName) {
        dst.m_roughness = src.m_roughness;
        dst.m_metallic = src.m_metallic;
        dst.m_texScale = glm::vec2{ 1, 1 };

        if ( !src.m_albedoMap.empty() ) {
            if ( ':' == src.m_albedoMap.front() ) {
                dst.m_diffuseMap = resMas.orderTexture((packageName + src.m_albedoMap).c_str(), true);
            }
            else {
                dst.m_diffuseMap = resMas.orderTexture(src.m_albedoMap.c_str(), true);
            }
        }
        if ( !src.m_roughnessMap.empty() ) {
            if ( ':' == src.m_roughnessMap.front() ) {
                dst.m_roughnessMap = resMas.orderTexture((packageName + src.m_roughnessMap).c_str(), false);
            }
            else {
                dst.m_roughnessMap = resMas.orderTexture(src.m_roughnessMap.c_str(), false);
            }
        }
        if ( !src.m_metallicMap.empty() ) {
            if ( ':' == src.m_metallicMap.front() ) {
                dst.m_metallicMap = resMas.orderTexture((packageName + src.m_metallicMap).c_str(), false);
            }
            else {
                dst.m_metallicMap = resMas.orderTexture(src.m_metallicMap.c_str(), false);
            }
        }

#if DAL_NORMAL_MAPPING
        if ( !src.m_normalMap.empty() ) {
            if ( ':' == src.m_normalMap.front() ) {
                dst.m_normalMap = resMas.orderTexture((packageName + src.m_normalMap).c_str(), false);
            }
            else {
                dst.m_normalMap = resMas.orderTexture(src.m_normalMap.c_str(), false);
            }
        }
#endif
    }

    void copyTransform(dal::Transform& dst, const dal::v1::cpnt::Transform src) {
        dst.setPos(src.m_pos);
        dst.setQuat(src.m_quat);
        dst.setScale(src.m_scale);
    }

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

    void MapChunk2::getWaters(std::vector<WaterRenderer*>& result) {
        for ( auto& water : this->m_waters ) {
            result.push_back(&water);
        }
    }


    void MapChunk2::applyCollision(const ICollider& inCol, cpnt::Transform& inTrans) {
        PhysicalProperty inPhysics, mdlPhysics;
        inPhysics.setMassInv(1.f);

        for ( auto& mdl : this->m_staticActors ) {
            const auto mdlBounding = mdl.m_model->getBounding();
            if ( nullptr == mdlBounding ) {
                continue;
            }

            const auto mdlDetailed = mdl.m_model->getDetailed();
            if ( nullptr != mdlDetailed ) {
                for ( auto& actor : mdl.m_actors ) {
                    const auto withBounding = checkCollisionAbs(inCol, *mdlBounding, inTrans, actor.m_transform);
                    if ( !withBounding ) {
                        continue;
                    }

                    const auto result = calcResolveInfoABS(inCol, inPhysics, inTrans, *mdlDetailed, mdlPhysics, actor.m_transform);
                    if ( result.m_valid ) {
                        inTrans.addPos(result.m_this);
                        actor.m_transform.addPos(result.m_other);
                    }
                }
            }
            else {
                for ( auto& actor : mdl.m_actors ) {
                    const auto result = calcResolveInfoABS(inCol, inPhysics, inTrans, *mdlBounding, mdlPhysics, actor.m_transform);
                    if ( result.m_valid ) {
                        inTrans.addPos(result.m_this);
                        actor.m_transform.addPos(result.m_other);
                    }
                }
            }
        }
    }

    std::optional<RayCastingResult> MapChunk2::castRayToClosest(const Segment& ray) const {
        float closestDistance = std::numeric_limits<float>::max();
        std::optional<RayCastingResult> result{ std::nullopt };

        for ( auto& modelActor : this->m_staticActors ) {
            const auto mdlBounding = modelActor.m_model->getBounding();
            if ( nullptr == mdlBounding ) {
                continue;
            }

            const auto mdlDetailed = modelActor.m_model->getDetailed();

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


    void MapChunk2::renderWater(const UnilocWaterry& uniloc) {
        this->sendLightUniforms(uniloc.m_lightedMesh, 0);

        for ( auto& water : this->m_waters ) {
            water.render(uniloc);
        }
    }


    void MapChunk2::render_static(const UniRender_Static& uniloc) {
        dalAssert(this->m_plights.size() <= 3);
        uniloc.i_lighting.plightCount(this->m_plights.size());
        for ( size_t i = 0; i < this->m_plights.size(); ++i ) {
            this->m_plights[i].sendUniform(i, uniloc.i_lighting);
        }

        for ( const auto& [model, actors] : this->m_staticActors ) {
            for ( const auto& actor : actors ) {
                uniloc.modelMat(actor.m_transform.getMat());
                model->render(uniloc);
            }
        }
    }

    void MapChunk2::render_animated(const UniRender_Animated& uniloc) {

    }

    void MapChunk2::render_staticDepth(const UniRender_StaticDepth& uniloc) {
        for ( const auto& [mdl, actors] : this->m_staticActors ) {
            for ( const auto& actor : actors ) {
                uniloc.modelMat(actor.m_transform.getMat());
                mdl->render(uniloc);
            }
        }
    }

    void MapChunk2::render_animatedDepth(const UniRender_AnimatedDepth& uniloc) {

    }


    int MapChunk2::sendLightUniforms(const UniInterfLightedMesh& uniloc, int startIndex) const {
        if ( startIndex >= 3 )
            dalAbort("Too many point lights.");
        if ( startIndex + this->m_plights.size() > 3 )
            dalAbort("Too many point lights.");

        uniloc.plightCount(startIndex + this->m_plights.size());
        for ( size_t i = 0; i < this->m_plights.size(); i++ ) {
            if ( i >= 3 ) {
                break;
            }
            else {
                this->m_plights.at(i).sendUniform(uniloc.u_plights[startIndex + i]);
            }
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

    Package::~Package(void) {

#ifdef _DEBUG
        for ( auto& [name, mdl] : this->m_models ) {
            if ( mdl.use_count() > 1 ) {
                dalWarn(fmt::format("On package \"{}\" destruction, a static model \"{}\" is being referenced: {}", this->m_name, name, mdl.use_count()));
            }
        }
        this->m_models.clear();

        for ( auto& [name, mdl] : this->m_animatedModels ) {
            if ( mdl.use_count() > 1 ) {
                dalWarn(fmt::format("On package \"{}\" destruction, a animated model \"{}\" is being referenced: {}", this->m_name, name, mdl.use_count()));
            }
        }
        this->m_animatedModels.clear();

        for ( auto& [name, mdl] : this->m_textures ) {
            if ( mdl.use_count() > 1 ) {
                dalWarn(fmt::format("On package \"{}\" destruction, a texture \"{}\" is being referenced: {}", this->m_name, name, mdl.use_count()));
            }
        }
#endif

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


    bool Package::hasTexture(const std::string& name) {
        return nullptr != this->getTexture(name);
    }

    bool Package::hasModelStatic(const std::string& name) {
        return nullptr != this->getModelStatic(name);
    }

    bool Package::hasModelAnim(const std::string& name) {
        return nullptr != this->getModelAnim(name);
    }

    std::shared_ptr<const ModelStatic> Package::getModelStatic(const std::string& name) {
        auto found = this->m_models.find(name);
        if ( this->m_models.end() == found ) {
            return nullptr;
        }
        else {
            return found->second;
        }
    }

    std::shared_ptr<const ModelAnimated> Package::getModelAnim(const std::string& name) {
        auto found = this->m_animatedModels.find(name);
        if ( this->m_animatedModels.end() == found ) {
            return nullptr;
        }
        else {
            return found->second;
        }
    }

    std::shared_ptr<const Texture> Package::getTexture(const std::string& name) {
        auto found = this->m_textures.find(name);
        if ( this->m_textures.end() == found ) {
            return nullptr;
        }
        else {
            return found->second;
        }
    }

    bool Package::giveModelStatic(const std::string& name, const std::shared_ptr<ModelStatic>& mdl) {
        if ( this->m_models.end() != this->m_models.find(name) ) {
            dalError(fmt::format(ERR_FORMAT_STR, "static model", this->m_name, name));
            return false;
        }
        else {
            this->m_models.emplace(name, mdl);
            return true;
        }
    }

    bool Package::giveModelAnim(const std::string& name, const std::shared_ptr<ModelAnimated>& mdl) {
        if ( this->m_animatedModels.end() != this->m_animatedModels.find(name) ) {
            dalError(fmt::format(ERR_FORMAT_STR, "animated model", this->m_name, name));
            return false;
        }
        else {
            this->m_animatedModels.emplace(name, mdl);
            return true;
        }
    }

    bool Package::giveTexture(const std::string& name, const std::shared_ptr<Texture>& tex) {
        if ( this->m_textures.end() != this->m_textures.find(name) ) {
            dalError(fmt::format(ERR_FORMAT_STR, "texture", this->m_name, name));
            return false;
        }
        else {
            this->m_textures.emplace(name, tex);
            return true;
        }
    }

}


// ResourceMaster
namespace dal {

    ResourceMaster::ResourceMaster(TaskMaster& taskMas)
        : m_task(taskMas)
    {

    }

    void ResourceMaster::notifyTask(std::unique_ptr<ITask> task) {
        dalAssert(nullptr != task.get());

        const auto taskTyp = g_taskManger.reportDone(task.get());

        if ( taskTyp == LoadTaskManger::ResTyp::model_static ) {
            auto loaded = reinterpret_cast<LoadTaskManger::TaskModelStatic*>(task.get());
            if ( !loaded->out_success ) {
                dalError(fmt::format("Failed to load model: {}", loaded->in_modelID));
                return;
            }

            {
                loaded->data_coresponding.setResID(std::move(loaded->in_modelID));
                loaded->data_coresponding.setDetailed(std::move(loaded->out_info.m_detailedCol));

                loaded->data_coresponding.clearRenderUnits();
                loaded->data_coresponding.reserveRenderUnits(loaded->out_info.m_model.m_renderUnits.size());
                for ( auto& unitInfo : loaded->out_info.m_model.m_renderUnits ) {
                    auto& unit = loaded->data_coresponding.newRenderUnit();

                    unit.m_mesh.buildData(
                        unitInfo.m_mesh.m_vertices.data(),
                        unitInfo.m_mesh.m_texcoords.data(),
                        unitInfo.m_mesh.m_normals.data(),
                        unitInfo.m_mesh.m_vertices.size() / 3
                    );
                    unit.m_name = unitInfo.m_name;

                    copyMaterial(unit.m_material, unitInfo.m_material, *this, loaded->data_package.getName());
                }

                loaded->data_coresponding.setBounding(std::unique_ptr<ICollider>{ new ColAABB{ loaded->out_info.m_model.m_aabb } });
            }
        }
        else if ( taskTyp == LoadTaskManger::ResTyp::texture ) {
            auto loaded = reinterpret_cast<LoadTaskManger::TaskTexture*>(task.get());
            if ( !loaded->out_success ) {
                dalError(fmt::format("Failed to load texture: {}", loaded->in_texID));
                return;
            }

            loaded->data_handle->init_diffuseMap(loaded->out_img);
            dalInfo(fmt::format("Texture loaded: {}", loaded->in_texID));
        }
        else if ( taskTyp == LoadTaskManger::ResTyp::model_animated ) {
            auto loaded = reinterpret_cast<LoadTaskManger::TaskModelAnimated*>(task.get());
            if ( !loaded->out_success ) {
                dalError(fmt::format("Failed to load model: {}", loaded->in_modelID));
                return;
            }

            loaded->data_coresponding.setResID(std::move(loaded->in_modelID));

            loaded->data_coresponding.setBounding(std::unique_ptr<ICollider>{new ColAABB{ loaded->out_info.m_model.m_aabb }});

            loaded->data_coresponding.setSkeletonInterface(std::move(loaded->out_info.m_model.m_joints));
            loaded->data_coresponding.setAnimations(std::move(loaded->out_info.m_animations));

            loaded->data_coresponding.clearRenderUnits();
            loaded->data_coresponding.reserveRenderUnits(loaded->out_info.m_model.m_renderUnits.size());
            for ( auto& unitInfo : loaded->out_info.m_model.m_renderUnits ) {
                auto& unit = loaded->data_coresponding.newRenderUnit();

                unit.m_mesh.buildData(
                    unitInfo.m_mesh.m_vertices.data(),
                    unitInfo.m_mesh.m_texcoords.data(),
                    unitInfo.m_mesh.m_normals.data(),
                    unitInfo.m_mesh.m_boneIndex.data(),
                    unitInfo.m_mesh.m_boneWeights.data(),
                    unitInfo.m_mesh.m_vertices.size() / 3
                );
                unit.m_name = unitInfo.m_name;

                copyMaterial(unit.m_material, unitInfo.m_material, *this, loaded->data_package.getName());
            }
        }
        else if ( taskTyp == LoadTaskManger::ResTyp::cube_map ) {
            auto loaded = reinterpret_cast<LoadTaskManger::TaskCubeMap*>(task.get());
            if ( !loaded->out_success ) {
                dalError(fmt::format("Failed to load cube map: {}", loaded->in_resIDs[0]));
                return;
            }

            CubeMap::CubeMapData data;

            for ( int i = 0; i < 6; ++i ) {
                auto& info = loaded->out_imgs[i];
                data.set(i, info.data(), info.width(), info.height(), info.pixSize());
            }

            loaded->data_handle->init(data);
        }
        else {
            dalWarn("ResourceMaster got a task that it doesn't know.");
        }
    }


    std::shared_ptr<const ModelStatic> ResourceMaster::orderModelStatic(const char* const respath) {
        const auto resinfo = parseResPath(respath);
        auto& package = this->orderPackage(resinfo.m_package);

        auto found = package.getModelStatic(resinfo.m_finalPath);
        if ( found ) {
            return found;
        }
        else {
            auto model = new ModelStatic; dalAssert(nullptr != model);
            std::shared_ptr<ModelStatic> modelHandle{ model };
            model->setResID(resinfo.m_finalPath); // It might not be resolved.
            package.giveModelStatic(resinfo.m_finalPath, modelHandle);

            auto task = g_taskManger.newModelStatic(respath, *model, package);
            this->m_task.orderTask(std::move(task), this);

            return modelHandle;
        }
    }

    std::shared_ptr<const ModelAnimated> ResourceMaster::orderModelAnim(const char* const respath) {
        const auto resinfo = parseResPath(respath);
        auto& package = this->orderPackage(resinfo.m_package);

        auto found = package.getModelAnim(resinfo.m_finalPath);
        if ( found ) {
            return found;
        }
        else {
            auto model = new ModelAnimated; dalAssert(nullptr != model);
            std::shared_ptr<ModelAnimated> modelHandle{ model };
            model->setResID(resinfo.m_finalPath);
            package.giveModelAnim(resinfo.m_finalPath, modelHandle);

            auto task = g_taskManger.newModelAnimated(respath, *model, package);
            this->m_task.orderTask(std::move(task), this);

            return modelHandle;
        }
    }

    std::shared_ptr<const Texture> ResourceMaster::orderTexture(const char* const respath, const bool gammaCorrect) {
        const auto resinfo = parseResPath(respath);
        auto& package = this->orderPackage(resinfo.m_package);

        auto found = package.getTexture(resinfo.m_finalPath);
        if ( nullptr != found ) {
            return found;
        }
        else {
            auto texture = std::shared_ptr<Texture>{ new Texture };
            package.giveTexture(resinfo.m_finalPath, texture);

            auto task = g_taskManger.newTexture(respath, texture.get(), gammaCorrect);
            this->m_task.orderTask(std::move(task), this);

            return texture;
        }
    }

    std::shared_ptr<const CubeMap> ResourceMaster::orderCubeMap(const std::array<std::string, 6>& respathes, const bool gammaCorrect) {
        auto tex = this->m_cubeMaps.emplace_back(new CubeMap);

        auto task = g_taskManger.newCubeMap(respathes, tex.get(), gammaCorrect);
        this->m_task.orderTask(std::move(task), this);

        return tex;
    }


    MapChunk2 ResourceMaster::loadMap(const char* const respath) {
        std::vector<uint8_t> buffer;
        loadFileBuffer(respath, buffer);
        auto mapInfo = parseDLB(buffer.data(), buffer.size());
        if ( !mapInfo ) {
            dalAbort(fmt::format("Failed to load map : {}", respath));
        }

        MapChunk2 map;

        for ( auto& mdlEmbed : mapInfo->m_embeddedModels ) {
            //std::shared_ptr<ModelStatic> model{ new ModelStatic };
            auto model = std::make_shared<ModelStatic>();

            // Name
            {
                model->setResID(mdlEmbed.m_name);
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

                copyMaterial(unit.m_material, unitInfo.m_material, *this, "");
            }

            // Colliders
            {
                model->setBounding(std::move(mdlEmbed.m_bounding));
                model->setDetailed(std::move(mdlEmbed.m_detailed));
            }

            map.addStaticActorModel(std::move(model), std::move(mdlEmbed.m_staticActors));
        }

        for ( auto& mdlImport : mapInfo->m_importedModels ) {
            auto model = this->orderModelStatic(mdlImport.m_resourceID.c_str());
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

    MapChunk2 ResourceMaster::loadChunk(const char* const respath) {
        const auto respathParsed = parseResPath(respath);

        std::vector<uint8_t> buffer;
        const auto loadResult = loadFileBuffer(respath, buffer);
        dalAssert(loadResult);

        auto mapInfo = parseMapChunk_v1(buffer.data(), buffer.size());
        if ( !mapInfo ) {
            dalAbort(fmt::format("failed to parse map chunk: {}", respath));
        }

        MapChunk2 map;

        for ( auto& unitInfo : mapInfo->m_renderUnits ) {
            auto model = std::make_shared<ModelStatic>();

            auto& unit = model->newRenderUnit();
            unit.m_mesh.buildData(
                unitInfo.m_mesh.m_vertices.data(),
                unitInfo.m_mesh.m_uvcoords.data(),
                unitInfo.m_mesh.m_normals.data(),
                unitInfo.m_mesh.m_vertices.size() / 3
            );

            copyMaterial(unit.m_material, unitInfo.m_material, *this, respathParsed.m_package);
            model->setBounding(std::unique_ptr<ICollider>{new ColAABB{ unitInfo.m_aabb.m_min, unitInfo.m_aabb.m_max }});

            map.m_staticActors.emplace_back(model);
        }

        for ( auto& sactorInfo : mapInfo->m_staticActors ) {
            auto& modelActor = map.m_staticActors[sactorInfo.m_modelIndex];
            modelActor.m_actors.emplace_back();

            modelActor.m_actors.back().m_name = sactorInfo.m_name;
            copyTransform(modelActor.m_actors.back().m_transform, sactorInfo.m_trans);
        }

        const auto win_width = GlobalStateGod::getinst().getWinWidth();
        const auto win_height = GlobalStateGod::getinst().getWinHeight();

        for ( auto& waterInfo : mapInfo->m_waters ) {
            dal::WaterRenderer::BuildInfo buildInfo;
            buildInfo.m_centerPos = waterInfo.m_centerPos;
            buildInfo.m_deepColor = waterInfo.m_deepColor;
            buildInfo.m_width = waterInfo.m_width;
            buildInfo.m_height = waterInfo.m_height;
            buildInfo.m_flowSpeed = waterInfo.m_flowSpeed;
            buildInfo.m_waveStreng = waterInfo.m_waveStreng;
            buildInfo.m_darkestDepth = waterInfo.m_darkestDepth;
            buildInfo.m_reflectance = waterInfo.m_reflectance;

            map.m_waters.emplace_back(buildInfo, win_width, win_height);
        }

        for ( auto& plightInfo : mapInfo->m_plights ) {
            auto& plight = map.m_plights.emplace_back();

            plight.mPos = plightInfo.m_pos;
            plight.m_color = plightInfo.m_color * plightInfo.m_intensity;
            plight.mMaxDistance = plightInfo.m_maxDist;
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
