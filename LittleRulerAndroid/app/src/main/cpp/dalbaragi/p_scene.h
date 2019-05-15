#pragma once

#include <vector>
#include <list>

#include "p_uniloc.h"
#include "u_loadinfo.h"
#include "p_resource.h"


namespace dal {

	class SceneMaster {

		//////// Definitions ////////

	private:
		struct ModelNActor {
			std::list<Actor> m_inst;
			ModelHandle m_model;
		};

		struct MapChunk {
			std::string m_name;
			std::vector<ModelNActor> m_modelActors;
		};

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

		Actor* addActorForModel(const ResourceID& resID, const std::string& actorName);

		void loadMap(const ResourceID& mapID);

	private:
		void addMap(const LoadedMap& map);

	};

}