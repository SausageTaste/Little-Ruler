#pragma once

#include <string>
#include <vector>
#include <deque>
#include <list>

#include <glm/gtc/quaternion.hpp>

#include "p_meshStatic.h"
#include "p_material.h"
#include "p_texture.h"
#include "p_uniloc.h"
#include "u_loadinfo.h"
#include "p_resource.h"


namespace dal {

	


	class SceneMaster {

	private:
		struct ModelNActor {
			std::list<Actor> m_inst;
			ModelHandle m_model;
		};

		struct MapChunk {
			std::vector<ModelNActor> m_actors;
		};

	private:
		//TextureMaster& m_texMas;

		std::list<MapChunk> m_mapChunks;

		ResourceMaster m_resMas;

	public:
		SceneMaster(void);
		~SceneMaster(void);

		void renderGeneral(const UnilocGeneral& uniloc) const;
		void renderDepthMp(const UnilocDepthmp& uniloc) const;

		void loadMap(const char* const mapID);

	private:
		void addMap(const LoadedMap& map);
	};

}