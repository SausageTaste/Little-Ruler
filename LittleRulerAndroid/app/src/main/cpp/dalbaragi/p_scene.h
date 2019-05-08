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
			std::vector<ModelNActor> m_actors;
		};

		//////// Attribs ////////

	private:
		ResourceMaster m_resMas;
		std::list<MapChunk> m_mapChunks;

		//////// Methods ////////

	public:
		SceneMaster(void);

		void renderGeneral(const UnilocGeneral& uniloc) const;
		void renderDepthMp(const UnilocDepthmp& uniloc) const;

		void loadMap(const char* const mapID);

	private:
		void addMap(const LoadedMap& map);

	};

}