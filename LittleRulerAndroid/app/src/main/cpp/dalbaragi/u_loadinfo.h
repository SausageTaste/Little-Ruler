#pragma once

#include <vector>
#include <string>
#include <list>

#include <glm/glm.hpp>


namespace dal {

	struct MeshInfo {
		std::vector<float> m_vertices, m_texcoords, m_normals;
		std::string m_name;
	};

	struct MaterialInfo {
		glm::vec3 m_diffuseColor;
		std::string m_diffuseMap, m_specularMap;
		float m_shininess = 32.0f, m_specStrength = 1.0f;
	};

	struct RenderUnitInfo {
		std::string m_name;
		MeshInfo m_mesh;
		MaterialInfo m_material;
	};

	using ModelInfo = std::list<RenderUnitInfo>;

}