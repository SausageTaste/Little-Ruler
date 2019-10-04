#pragma once

#include <array>
#include <optional>
#include <unordered_map>

#include <entt/entity/registry.hpp>

#include "s_threader.h"
#include "u_fileclass.h"
#include "p_meshStatic.h"
#include "p_water.h"
#include "p_model.h"
#include "p_light.h"
#include "p_skybox.h"


namespace dal {

    class MapChunk2 {

    private:
        struct StaticModelActor {
            std::shared_ptr<const ModelStatic> m_model;
            std::vector<ActorInfo> m_actors;

            StaticModelActor(std::shared_ptr<const ModelStatic>&& model, std::vector<ActorInfo>&& actors)
                : m_model(std::move(model))
                , m_actors(std::move(actors))
            {

            }
        };

    private:
        std::vector<StaticModelActor> m_staticActors;
        std::vector<WaterRenderer> m_waters;
        std::vector<PointLight> m_plights;

    public:
        void onWinResize(const unsigned int winWidth, const unsigned int winHeight);

        void addStaticActorModel(std::shared_ptr<const ModelStatic>&& model, std::vector<ActorInfo>&& actors) {
            this->m_staticActors.emplace_back(std::move(model), std::move(actors));
        }
        void addWaterPlane(const dlb::WaterPlane& waterInfo);
        PointLight& newPlight(void) {
            return this->m_plights.emplace_back();
        }

        void applyCollision(const ICollider& inCol, cpnt::Transform& inTrans);
        std::optional<RayCastingResult> castRayToClosest(const Ray& ray) const;

        void renderGeneral(const UnilocGeneral& uniloc);
        void renderDepthGeneral(const UnilocDepthmp& uniloc);
        void renderWater(const UnilocWaterry& uniloc);
        void renderOnWaterGeneral(const UnilocGeneral& uniloc, const ICamera& cam, entt::registry& reg);
        void renderOnWaterAnimated(const UnilocAnimate& uniloc, const ICamera& cam, entt::registry& reg);
        void renderOnWaterSkybox(const UnilocSkybox& uniloc, const Skybox& skybox, const ICamera& cam);

    private:
        int sendLightUniforms(const UniInterfLightedMesh& uniloc, int startIndex) const;

    };


    class Package {

    private:
        std::string m_name;
        // All keys are stored in form of file name + ext.
        std::unordered_map<std::string, std::shared_ptr<ModelStatic>> m_models;
        std::unordered_map<std::string, std::shared_ptr<ModelAnimated>> m_animatedModels;
        std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;

    public:
        Package(const Package&) = delete;
        Package& operator=(const Package&) = delete;

    public:
        Package(const std::string& pckName);
        Package(std::string&& pckName);
        ~Package(void);

        Package(Package&& other) noexcept;
        Package& operator=(Package&&) noexcept;

        bool hasTexture(const ResourceID& resPath);
        bool hasModelStatic(const ResourceID& resPath);
        bool hasModelAnim(const ResourceID& resPath);

        std::shared_ptr<const ModelStatic> getModelStatic(const ResourceID& resID);
        std::shared_ptr<const ModelAnimated> getModelAnim(const ResourceID& resID);
        std::shared_ptr<const Texture> getTexture(const ResourceID& resID);

        bool giveModelStatic(const ResourceID& resID, const std::shared_ptr<ModelStatic>& mdl);
        bool giveModelAnim(const ResourceID& resID, const std::shared_ptr<ModelAnimated>& mdl);
        bool giveTexture(const ResourceID& resID, const std::shared_ptr<Texture>& tex);

    };


    class ResourceMaster : public ITaskDoneListener {

        //////// Attribs ////////

    private:
        std::unordered_map<std::string, Package> m_packages;
        std::vector<std::shared_ptr<CubeMap>> m_cubeMaps;

        //////// Methods ////////

    public:
        ResourceMaster(const ResourceMaster&) = delete;
        ResourceMaster& operator=(const ResourceMaster&) = delete;

    public:
        ResourceMaster(void) = default;
        virtual ~ResourceMaster(void) override = default;

        virtual void notifyTask(std::unique_ptr<ITask> task) override;

        std::shared_ptr<const ModelStatic> orderModelStatic(const ResourceID& resID);
        std::shared_ptr<const ModelAnimated> orderModelAnim(const ResourceID& resID);
        std::shared_ptr<const Texture> orderTexture(const ResourceID& resID, const bool gammaCorrect);
        std::shared_ptr<const CubeMap> orderCubeMap(const std::array<ResourceID, 6>& resIDs, const bool gammaCorrect);

        MapChunk2 loadMap(const ResourceID& resID);

    private:
        Package& orderPackage(const std::string& packName);

    };

}
