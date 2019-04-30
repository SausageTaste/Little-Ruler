#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <list>

#include <glm/glm.hpp>


namespace dal {

	struct OBJObjectInfo_deprecated {
		std::vector<float> mVertices, mTexcoords, mNormals;
		std::vector<glm::vec3> debugVertices, debugNormals;
		std::vector<glm::vec2> debugTexcoords;
		std::string mName, mMaterialName;

		OBJObjectInfo_deprecated(const char* const name) : mName(name) {

		}
	};

	struct OBJInfo_deprecated {
		std::vector<OBJObjectInfo_deprecated> mObjects;
		std::vector<std::string> mMaterialFiles;
	};

	struct MTLMaterialInfo_deprecated {
		glm::vec3 mDiffuseColor;
		std::string mDiffuseMap, m_specularMap, mName;

		MTLMaterialInfo_deprecated(const char* const name) : mName(name) {

		}
	};

	struct MTLInfo_deprecated {
		std::vector<MTLMaterialInfo_deprecated> mMateirals;
	};


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


	bool parseOBJ_deprecated(OBJInfo_deprecated* const con, const uint8_t* const buf, const size_t bufSize);

	bool parseMTL_deprecated(MTLInfo_deprecated* info, const uint8_t* const buf, const size_t bufSize);

	bool parseOBJ_assimp(ModelInfo& info, const char* const assetPath);

}