#pragma once

#include <array>
#include <optional>
#include <unordered_map>

#include <entt/entity/registry.hpp>

#include "s_threader.h"
#include "p_meshStatic.h"
#include "p_water.h"
#include "p_model.h"
#include "p_light.h"


namespace dal {

    class MapChunk2 {

    private:
        struct StaticModelActor {
            std::shared_ptr<const ModelStatic> m_model;
            std::vector<ActorInfo> m_actors;

            StaticModelActor(std::shared_ptr<const ModelStatic>&& model) 
                : m_model(std::move(model)) 
            {

            }
            StaticModelActor(std::shared_ptr<const ModelStatic>&& model, std::vector<ActorInfo>&& actors)
                : m_model(std::move(model))
                , m_actors(std::move(actors))
            {

            }
        };

    public:
        std::vector<StaticModelActor> m_staticActors;
        std::vector<WaterRenderer> m_waters;
        std::vector<PointLight> m_plights;
        std::vector<SpotLight> m_slights;

    public:
        void onWinResize(const unsigned int winWidth, const unsigned int winHeight);

        void addStaticActorModel(std::shared_ptr<const ModelStatic>&& model, std::vector<ActorInfo>&& actors) {
            this->m_staticActors.emplace_back(std::move(model), std::move(actors));
        }
        void addWaterPlane(const dlb::WaterPlane& waterInfo);
        void getWaters(std::vector<WaterRenderer*>& result);
        PointLight& newPlight(void) {
            return this->m_plights.emplace_back();
        }

        void applyCollision(const ICollider& inCol, cpnt::Transform& inTrans);
        std::optional<RayCastingResult> castRayToClosest(const Segment& ray) const;

        void renderWater(const UnilocWaterry& uniloc);

        void render_static(const UniRender_Static& uniloc);
        void render_animated(const UniRender_Animated& uniloc);
        void render_staticDepth(const UniRender_StaticDepth& uniloc);
        void render_animatedDepth(const UniRender_AnimatedDepth& uniloc);

        int sendLightUniforms(const UniInterfLightedMesh& uniloc, int startIndex) const;

    };


    class Package {

    private:
        std::string m_name;
        // All keys are stored in form of filename + ext.
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

        const std::string& getName(void) const {
            return this->m_name;
        }

        bool hasTexture(const std::string& name);
        bool hasModelStatic(const std::string& name);
        bool hasModelAnim(const std::string& name);

        std::shared_ptr<const ModelStatic> getModelStatic(const std::string& name);
        std::shared_ptr<const ModelAnimated> getModelAnim(const std::string& name);
        std::shared_ptr<const Texture> getTexture(const std::string& name);

        bool giveModelStatic(const std::string& name, const std::shared_ptr<ModelStatic>& mdl);
        bool giveModelAnim(const std::string& name, const std::shared_ptr<ModelAnimated>& mdl);
        bool giveTexture(const std::string& name, const std::shared_ptr<Texture>& tex);

    };


    class ResourceMaster : public ITaskDoneListener {

        //////// Attribs ////////

    private:
        TaskMaster& m_task;

        std::unordered_map<std::string, Package> m_packages;
        std::vector<std::shared_ptr<CubeMap>> m_cubeMaps;

        //////// Methods ////////

    public:
        ResourceMaster(const ResourceMaster&) = delete;
        ResourceMaster& operator=(const ResourceMaster&) = delete;

    public:
        ResourceMaster(TaskMaster& taskMas);
        virtual ~ResourceMaster(void) override = default;

        virtual void notifyTask(std::unique_ptr<ITask> task) override;

        std::shared_ptr<const ModelStatic> orderModelStatic(const char* const respath);
        std::shared_ptr<const ModelAnimated> orderModelAnim(const char* const respath);
        std::shared_ptr<const Texture> orderTexture(const char* const respath, const bool gammaCorrect);
        std::shared_ptr<const CubeMap> orderCubeMap(const std::array<std::string, 6>& respathes, const bool gammaCorrect);

        MapChunk2 loadMap(const char* const respath);
        MapChunk2 loadChunk(const char* const respath);

    private:
        Package& orderPackage(const std::string& packName);

    };

}
