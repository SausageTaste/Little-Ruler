#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <list>

#include <glm/glm.hpp>

#include "u_loadinfo.h"


namespace dal_deprecated {

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


	bool parseOBJ_deprecated(OBJInfo_deprecated* const con, const uint8_t* const buf, const size_t bufSize);

	bool parseMTL_deprecated(MTLInfo_deprecated* info, const uint8_t* const buf, const size_t bufSize);
}


namespace dal {

	bool parseOBJ_assimp(ModelInfo& info, const char* const assetPath);

}