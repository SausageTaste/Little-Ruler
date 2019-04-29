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


namespace dal {

	class RenderUnit_Static {

	public:
		MeshStatic mesh;
		Material material;

		bool isReady(void) const;
	};


	class ModelInstanceInfo {

	public:
		std::string m_instName;
		glm::vec3 pos;
		glm::quat myQuat;
		float rescale;

		ModelInstanceInfo(void);
		ModelInstanceInfo(glm::vec3 initPos);
		ModelInstanceInfo(const float x, const float y , const float z);
		void getViewMat(glm::mat4* mat) const;
		void rotate(const float v, const glm::vec3& selector);

	};


	struct Pair_MeshMaterial {
		std::string m_name;
		MeshStatic m_mesh;
		Material m_material;
	};


	struct ModelInst {
		std::string m_name;
		std::deque<Pair_MeshMaterial> m_renderUnits;
		std::list<ModelInstanceInfo> m_inst;
	};


	struct ModelBuildInfo_AABB {
		const char* m_modelName = nullptr;
		const char* m_textureName = nullptr;
		glm::vec3 m_p1, m_p2;
		float m_shininess = 32.0f, m_specStrength = 1.0f;
		float m_texScaleX = 1.0f, m_texScaleY = 1.0f;
		std::list<ModelInstanceInfo> m_instanceInfo;
	};

	struct ModelBuildInfo_Load {
		const char* m_modelName;
		std::list<ModelInstanceInfo> m_instanceInfo;
	};


	class SceneMaster : iTaskDoneListener {

	private:
		TextureMaster& m_texMas;

		std::deque<ModelInst> m_freeModels;

		RenderUnit_Static mLoadedUnits[120];
		std::list<ModelInstanceInfo> mLoadedInsts[120];

	public:
		SceneMaster(TextureMaster& texMas);
		~SceneMaster(void);

		virtual void notify(iTask* const task) override;

		void renderGeneral(const UnilocGeneral& uniloc) const;
		void renderDepthMp(const UnilocDepthmp& uniloc) const;

		void addObject(const ModelBuildInfo_AABB& info);
		void addObject(const ModelBuildInfo_Load& info);

	private:
		bool findModel(const char* const name, ModelInst** model, const char** level);

	};

}