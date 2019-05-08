#pragma once

#include <vector>
#include <string>
#include <list>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


namespace dal {

	struct Actor {
		std::string m_actorID;
		glm::vec3 pos;
		glm::quat myQuat;

		glm::mat4 getViewMat(void) const;
		void rotate(const float v, const glm::vec3& selector);
	};

	struct RenderUnitInfo {

		struct MeshInfo {
			std::vector<float> m_vertices, m_texcoords, m_normals;
			std::string m_name;
		};

		struct MaterialInfo {
			glm::vec3 m_diffuseColor;
			std::string m_diffuseMap, m_specularMap;
			float m_shininess = 32.0f, m_specStrength = 1.0f;
		};

		std::string m_name;
		MeshInfo m_mesh;
		MaterialInfo m_material;

	};


	namespace loadedinfo {

		struct MapItemModel {
			std::string m_modelID;
			std::vector<Actor> m_actors;
		};

		struct ModelDefined : public MapItemModel {
			RenderUnitInfo m_renderUnit;
		};

		struct ModelImported : public MapItemModel {

		};

		struct ImageFileData {
			std::vector<uint8_t> m_buf;
			size_t m_width = 0, m_height = 0, m_pixSize = 0;
		};

	}


	struct LoadedMap {

		std::string m_mapName, m_packageName;
		std::list<loadedinfo::ModelDefined> m_definedModels;
		std::list<loadedinfo::ModelImported> m_importedModels;

	};

	using ModelInfo = std::list<RenderUnitInfo>;

}