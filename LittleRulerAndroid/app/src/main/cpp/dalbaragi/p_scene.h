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
#include "s_threader.h"
#include "u_loadinfo.h"
#include "p_resource.h"


namespace dal {

	struct RenderUnit_Static {
		MeshStatic mesh;
		Material material;

		bool isReady(void) const;
	};


	struct Pair_MeshMaterial {
		std::string m_name;
		MeshStatic m_mesh;
		Material m_material;
	};


	struct ModelInst {
		std::string m_name;
		std::deque<Pair_MeshMaterial> m_renderUnits;
		std::list<Actor> m_inst;
	};


	struct ModelBuildInfo_AABB {
		const char* m_modelName = nullptr;
		const char* m_textureName = nullptr;
		glm::vec3 m_p1, m_p2;
		float m_shininess = 32.0f, m_specStrength = 1.0f;
		float m_texScaleX = 1.0f, m_texScaleY = 1.0f;
		std::list<Actor> m_instanceInfo;
	};

	struct ModelBuildInfo_Load {
		const char* m_modelName = nullptr;
		std::list<Actor> m_instanceInfo;
	};


	class SceneMaster : iTaskDoneListener {

	private:
		struct MapChunk {
			std::deque<ModelInst> m_actors;
		};

		TextureMaster& m_texMas;

		std::deque<ModelInst> m_freeModels;
		std::list<MapChunk> m_mapChunks;

		std::unordered_set<ModelInst*> m_modelsNotComplete;

	public:
		SceneMaster(TextureMaster& texMas);
		~SceneMaster(void);

		virtual void notify(iTask* const task) override;

		void renderGeneral(const UnilocGeneral& uniloc) const;
		void renderDepthMp(const UnilocDepthmp& uniloc) const;

		void addObject(const ModelBuildInfo_AABB& info);
		void addObject(const ModelBuildInfo_Load& info);
		
		void addMapChunk(const LoadedMap& map);

	private:
		bool findModel(const char* const name, ModelInst** model, const char** level);

	};

}