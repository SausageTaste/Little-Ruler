#pragma once

#include <string>
#include <vector>
#include <list>

#include "p_uniloc.h"
#include "u_loadinfo.h"
#include "p_resource.h"
#include "p_light.h"
#include "p_water.h"


namespace dal {

	class MapChunk {

	private:
		struct ModelNActor {
			Model* m_model = nullptr;
			std::list<ActorInfo> m_inst;

			ModelNActor(void) = default;
			ModelNActor(Model* const model) : m_model(model) {}
		};

	private:
		std::string m_name;

		std::vector<ModelNActor> m_modelActors;

		std::vector<DirectionalLight> m_dlights;
		std::vector<PointLight> m_plights;

		std::vector<WaterRenderer> m_waters;

	public:
		MapChunk(const std::string& name);
		MapChunk(const LoadedMap& info, ResourceMaster& resMan);

		const std::string& getName(void) const;

		void onScreanResize(const unsigned int width, const unsigned int height);

		void renderGeneral(const UnilocGeneral& uniloc) const;
		void renderDepthMp(const UnilocDepthmp& uniloc) const;
		void renderWaterry(const UnilocWaterry& uniloc);
		void renderGeneral_onWater(const UnilocGeneral& uniloc, const Camera& cam, MapChunk* const additional);

		int sendUniforms_lights(const UnilocGeneral& uniloc, int startIndex) const;
		int sendUniforms_lights(const UnilocWaterry& uniloc, int startIndex) const;

		WaterRenderer* getWater(const size_t index);
		ActorInfo* addActor(Model* const model, const std::string& actorName, bool flagStatic, ResourceMaster& resMas);

	};

	class SceneMaster {

		//////// Attribs ////////

	private:
		ResourceMaster& m_resMas;
		std::list<MapChunk> m_mapChunks;
		MapChunk* m_persistantMap;

		//////// Methods ////////

	public:
		SceneMaster(ResourceMaster& resMas);
		~SceneMaster(void);

		void renderGeneral(const UnilocGeneral& uniloc) const;
		void renderDepthMp(const UnilocDepthmp& uniloc) const;
		void renderWaterry(const UnilocWaterry& uniloc);
		void renderGeneral_onWater(const UnilocGeneral& uniloc, const Camera& cam);

		ActorInfo* addActor(Model* const model, const std::string& mapName, const std::string& actorName, bool flagStatic);
		WaterRenderer* getWater(const std::string& mapName, const size_t index);

		void loadMap(const ResourceID& mapID);

		void onResize(const unsigned int width, const unsigned int height);

	private:
		void addMap(const LoadedMap& map);
		MapChunk* findMap(const std::string& name);

	};

}