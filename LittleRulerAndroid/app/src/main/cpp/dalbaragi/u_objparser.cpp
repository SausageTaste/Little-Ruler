#include "u_objparser.h"

#include <cstring>
#include <string>
#include <array>
#include <vector>
#include <iostream>

#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>

#include "s_logger_god.h"
#include "u_fileclass.h"

#include <cstdio>


using namespace std::string_literals;


namespace {

	class AssIOStreamAsset : public Assimp::IOStream {

	private:
		dal::AssetFileStream m_handle;

	public:
		AssIOStreamAsset(void) = default;

		bool open(const char* const path) {
			return m_handle.open(path);
		}

		virtual size_t FileSize(void) const override {
			return m_handle.getFileSize();
		}

		virtual void Flush(void) override {

		}

		virtual size_t Read(void* pvBuffer, size_t pSize, size_t pCount) override {
			return m_handle.read(static_cast<uint8_t*>(pvBuffer), pSize* pCount);
		}

		virtual aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override {
			dal::AssetFileStream::Whence whence;

			switch (pOrigin) {

			case aiOrigin_SET:
				whence = dal::AssetFileStream::Whence::beg;
				break;
			case aiOrigin_CUR:
				whence = dal::AssetFileStream::Whence::cur;
				break;
			case aiOrigin_END:
				whence = dal::AssetFileStream::Whence::end;
				break;
			default:
				dal::LoggerGod::getinst().putError("Invalid pOrigin value for AssIOStreamAsset::Seek: "s + std::to_string(pOrigin));
				return aiReturn_FAILURE;

			}

			return m_handle.seek(pOffset, whence) ? aiReturn_SUCCESS : aiReturn_FAILURE;
		}

		virtual size_t Tell() const override {
			return m_handle.tell();
		}

		virtual size_t Write(const void* pvBuffer, size_t pSize, size_t pCount) override {
			dal::LoggerGod::getinst().putError("Writing is not supported for assets.");
			throw - 1;
		}

	};


	class AssIOSystem_Asset : public Assimp::IOSystem {

	private:
		std::vector<std::string> m_stackDir;
		static const std::string m_emptyStr;

	public:
		virtual void Close(Assimp::IOStream* pFile) override {
			delete pFile;
		}

		virtual bool Exists(const char* pFile) const override {
			dal::AssetFileStream file;
			return file.open(pFile);
		}

		virtual char getOsSeparator() const override {
			return '/';
		}

		virtual Assimp::IOStream* Open(const char* pFile, const char* pMode = "rb") {
			if (pMode[0] == 'w') {
				dal::LoggerGod::getinst().putError("Writing is not supported for assets.");
				return nullptr;
			}

			auto file = new AssIOStreamAsset();
			if (file->open(pFile)) {
				return file;
			}
			else {
				delete file;
				return nullptr;
			}

		}

	};

}


namespace {  // Common

	auto& gLogger = dal::LoggerGod::getinst();

	unsigned int getIndexOfMostSimilar(const char* const str, const char *const *const map, size_t mapSize) {
		std::vector<bool> excludeMap;
		excludeMap.resize(mapSize);

		const auto len = strlen(str) + 1;
		for (unsigned int i = 0; i < len; i++) {
			for (unsigned int j = 0; j < mapSize; j++) {
				if (excludeMap[j] == true) continue;
				if (str[i] != map[j][i])  excludeMap[j] = true;
			}

			if (str[i] == ' ')  break;
		}

		// Count how may of excludeMap is false.
		int numOfNotExcluded = 0; {
			for (unsigned int j = 0; j < mapSize; j++) {
				if (excludeMap[j] == false) numOfNotExcluded++;
			}
		}

		if (numOfNotExcluded == 1) {  // If only one had survived, that one would be solution.
			for (unsigned int j = 0; j < mapSize; j++) {
				if (!excludeMap[j])  return j;
			}
			throw -1;  // numOfNotExcluded is 1 but no one is found is the uper for loop? This won't happen.
		}
		else if (numOfNotExcluded == 0) {  // If everything had been excluded, that was no match.
			//gLogger.putError("Unknown header str: "s + str);
			return -1;  // -1 means not found.
		}
		else {  // Two or more survived, cannot resolve.
			//gLogger.putError("Multiple match str: "s + str);
			return -1;  // -1 means not found.
		}
	}

	size_t findMaterialInOBJ(const std::string& fileContents, std::vector<std::string>& output) {
		unsigned int lastTail = 0;

		while (true) {
			const auto head = fileContents.find("mtllib ", lastTail) + 7;
			if (head == std::string::npos) break;
			const auto tail = fileContents.find("\n", head - 7);  // I have absolutely no idea why I need to subtract 7 here.
			if (tail == std::string::npos) break;

			output.push_back(fileContents.substr(head, tail - head));
			lastTail = tail;
		}
		
		return output.size();
	}

}


namespace {  // For mtl file

	enum class MTL_HeadOfLine {
		def_diffuseColor,
		def_diffuseMap,
		def_material,
		eof
	};

	constexpr unsigned int MTL_MAP_SIZE = int(MTL_HeadOfLine::eof) - int(MTL_HeadOfLine::def_diffuseColor);
	static const char* MTL_MAP[MTL_MAP_SIZE] = { "Kd ", "map_Kd ", "newmtl " };

}


namespace {

	bool isNumberCpnt(const char c) {
		if ('0' <= c && c <= '9') return true;
		if (c == '-')             return true;
		if (c == '.')             return true;

		return false;
	}

	enum class OBJ_HeadOfLine {
		def_vertex,    // v
		def_texcoord,  // vt
		def_normal,    // vn
		new_polygon,    // f
		comment,       // #
		def_object,    // o
		def_material,  // mtllib
		use_material,  // usemtl
		eof
	};

	constexpr unsigned int OBJ_MAP_SIZE = int(OBJ_HeadOfLine::eof) - int(OBJ_HeadOfLine::def_vertex);
	static const char* OBJ_MAP[OBJ_MAP_SIZE] = { "v ", "vt ", "vn ", "f ", "# ", "o ", "mtllib ", "usemtl " };

	size_t parseAllNumbersInLine(const char* const str, float* buf, size_t bufSize) {
		/*
		returns number of numbers in the line.
		Pass null to buf to only get buffer size.
		*/

		char numBuf[256];
		unsigned int numBufIndex = 0;
		unsigned int inputBufIndex = 0;

		size_t foundNumOfNum = 0;

		const auto len = strlen(str);
		for (unsigned int i = 0; i < len; i++) {
			const auto c = str[i];

			if (numBufIndex == 0) {  // This means no number is being precessed wet.
				if (isNumberCpnt(c)) {
					numBuf[numBufIndex] = c;
					numBufIndex++;
				}
			}
			else {
				if (isNumberCpnt(c)) {
					numBuf[numBufIndex] = c;
					numBufIndex++;
				}
				else {  // Fetching number had ended.
					numBuf[numBufIndex] = '\0';
					numBufIndex = 0;
					foundNumOfNum++;
					if (buf != nullptr) {
						if (inputBufIndex >= bufSize) {
							gLogger.putWarn("Buffer is full in ::parseAllNumbersInLine."); continue;
						}
						else {
							buf[inputBufIndex] = std::stof(numBuf);
							inputBufIndex++;
						}
					}
				}
			}
		}

		if (numBufIndex != 0) {  // It is possible that line ends with number.
			numBuf[numBufIndex] = '\0';
			numBufIndex = 0;
			foundNumOfNum++;
			if (buf != nullptr) {
				if (inputBufIndex >= bufSize) {
					gLogger.putWarn("Buffer is full in ::parseAllNumbersInLine.");
				}
				else {
					buf[inputBufIndex] = std::stof(numBuf);
					inputBufIndex++;
				}
			}
		}

		return foundNumOfNum;
	}

	size_t parseAllNumbersInLine(const char* const str, int* buf, size_t bufSize) {
		/*
		returns number of numbers in the line.
		Pass null to buf to only get buffer size.
		*/

		char numBuf[256];
		unsigned int numBufIndex = 0;
		unsigned int inputBufIndex = 0;

		size_t foundNumOfNum = 0;

		const auto len = strlen(str);
		for (unsigned int i = 0; i < len; i++) {
			const auto c = str[i];

			if (numBufIndex == 0) {  // This means no number is being precessed wet.
				if (isNumberCpnt(c)) {
					numBuf[numBufIndex] = c;
					numBufIndex++;
				}
			}
			else {
				if (isNumberCpnt(c)) {
					numBuf[numBufIndex] = c;
					numBufIndex++;
				}
				else {  // Fetching number had ended.
					numBuf[numBufIndex] = '\0';
					numBufIndex = 0;
					foundNumOfNum++;
					if (buf != nullptr) {
						if (inputBufIndex >= bufSize) {
							gLogger.putWarn("Buffer is full in ::parseAllNumbersInLine."); continue;
						}
						else {
							buf[inputBufIndex] = std::stoi(numBuf);
							inputBufIndex++;
						}
					}
				}
			}
		}

		if (numBufIndex != 0) {  // It is possible that line ends with number.
			numBuf[numBufIndex] = '\0';
			numBufIndex = 0;
			foundNumOfNum++;
			if (buf != nullptr) {
				if (inputBufIndex >= bufSize) {
					gLogger.putWarn("Buffer is full in ::parseAllNumbersInLine.");
				}
				else {
					buf[inputBufIndex] = std::stoi(numBuf);
					inputBufIndex++;
				}
			}
		}

		return foundNumOfNum;
	}

	int parseOneStrInLine(const char* const str, char* strBuf, size_t bufSize) {
		bool isFetching = false;
		int startIndex = -1;

		const auto len = strlen(str);
		for (unsigned int i = 0; i < len; i++) {
			if (str[i] == ' ') {
				startIndex = i + 1;
				break;
			}
		}

		if (startIndex <= 0) {
			return -1;
		}

		if (bufSize <= len) {
			gLogger.putFatal("Sorry I'm to lazy to make this optimized. :(");
			throw -1;  // To make this function not dangerous.
		}

		for (unsigned int i = startIndex; i < len; i++) {
			const auto bufIndex = i - startIndex;
			if (bufIndex >= bufSize) {
				strBuf[bufSize - 1] = '\0';
				return len - startIndex;
			}

			strBuf[bufIndex] = str[i];
			strBuf[bufIndex + 1] = '\0';  // Looks really dangerous lol!
		}

		// Dangerouse!! If buf size and found str size is similar, str will not be null terminated!
		// For now I'mma just make enough size of buffer, because I'm so tired. :(

		return len - startIndex;
	}

}


namespace {
	
	unsigned int processMaterial(const aiScene* const scene, std::vector<dal::RenderUnitInfo::MaterialInfo>& materials) {
		for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
			const auto iMaterial = scene->mMaterials[i];
			materials.emplace_back();
			auto& iMatInfo = materials.back();

			{
				float floatBuf;

				if (aiReturn_SUCCESS == aiGetMaterialFloat(iMaterial, AI_MATKEY_SHININESS, &floatBuf)) {
					iMatInfo.m_shininess = floatBuf;
				}

				if (aiReturn_SUCCESS == aiGetMaterialFloat(iMaterial, AI_MATKEY_SHININESS_STRENGTH, &floatBuf)) {
					iMatInfo.m_specStrength = floatBuf;
				}

				aiColor4D vec4Buf;
				if (aiReturn_SUCCESS == aiGetMaterialColor(iMaterial, AI_MATKEY_COLOR_DIFFUSE, &vec4Buf)) {
					iMatInfo.m_diffuseColor.r = vec4Buf.r;
					iMatInfo.m_diffuseColor.g = vec4Buf.g;
					iMatInfo.m_diffuseColor.b = vec4Buf.b;
				}
			}

			for (unsigned int j = 0; j < iMaterial->GetTextureCount(aiTextureType_DIFFUSE); j++) {
				aiString str;
				if (iMaterial->GetTexture(aiTextureType_DIFFUSE, j, &str) == aiReturn_SUCCESS) {
					iMatInfo.m_diffuseMap = str.C_Str();
				}
				
				break;  // Because it supports only one diffuse map atm.
			}

			for (unsigned int j = 0; j < iMaterial->GetTextureCount(aiTextureType_SPECULAR); j++) {
				aiString str;
				if (iMaterial->GetTexture(aiTextureType_SPECULAR, j, &str) == aiReturn_SUCCESS) {
					iMatInfo.m_specularMap = str.C_Str();
				}
				
				break;  // Because it supports only one specular map atm.
			}
		}

		return scene->mNumMaterials;
	}

	bool processMesh(dal::RenderUnitInfo& renUnit, aiMesh* const mesh) {
		renUnit.m_name = reinterpret_cast<char*>(&mesh->mName);
		
		renUnit.m_mesh.m_vertices.reserve (mesh->mNumVertices * 3);
		renUnit.m_mesh.m_texcoords.reserve(mesh->mNumVertices * 2);
		renUnit.m_mesh.m_normals.reserve  (mesh->mNumVertices * 3);

		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			auto vertex = mesh->mVertices[i];
			renUnit.m_mesh.m_vertices.push_back(vertex.x);
			renUnit.m_mesh.m_vertices.push_back(vertex.y);
			renUnit.m_mesh.m_vertices.push_back(vertex.z);

			auto texCoord = mesh->mTextureCoords[0][i];
			renUnit.m_mesh.m_texcoords.push_back(texCoord.x);
			renUnit.m_mesh.m_texcoords.push_back(texCoord.y);

			auto normal = mesh->mNormals[i];
			renUnit.m_mesh.m_normals.push_back(normal.x);
			renUnit.m_mesh.m_normals.push_back(normal.y);
			renUnit.m_mesh.m_normals.push_back(normal.z);
		}
		
		return true;
	}

	bool processNode(dal::ModelInfo& info, std::vector<dal::RenderUnitInfo::MaterialInfo> materials, const aiScene* const scene, aiNode* const node) {
		for (unsigned int i = 0; i < node->mNumMeshes; i++) {
			aiMesh* ai_mesh = scene->mMeshes[node->mMeshes[i]];
			info.emplace_back();
			auto& renUnit = info.back();
			if ( !processMesh(renUnit, ai_mesh) ) return false;

			if (ai_mesh->mMaterialIndex != 0) {
				renUnit.m_material = materials.at(ai_mesh->mMaterialIndex);
			}
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++) {
			if (processNode(info, materials, scene, node->mChildren[i]) == false) return false;
		}

		return true;
	}
	
}


namespace dal_deprecated {

	bool parseOBJ_deprecated(OBJInfo_deprecated* const con, const uint8_t* const buf, const size_t bufSize) {
		con->mMaterialFiles.clear();
		con->mObjects.clear();

		std::array<char, 256> bufLine;
		unsigned int curIndex = 0;

		std::vector<glm::vec3> vertices, normals;
		std::vector<glm::vec2> texcoords;
		OBJObjectInfo_deprecated dummyObj{ "dummy" };
		OBJObjectInfo_deprecated* curObject = &dummyObj;

		// Fetch all needed data
		for (unsigned int i = 0; i < bufSize; i++) {
			if (buf[i] == '\n') {
				bufLine[curIndex] = '\0';
				const char* const line = &bufLine[0];
				/* Precess a line */ {
					const auto head = OBJ_HeadOfLine(getIndexOfMostSimilar(line, OBJ_MAP, OBJ_MAP_SIZE) + int(OBJ_HeadOfLine::def_vertex));

					if (head == OBJ_HeadOfLine::def_vertex) {
						constexpr size_t arrSize = 3;
						float numBuf[arrSize];

						const auto numOfNums = parseAllNumbersInLine(line, numBuf, arrSize);
						assert(numOfNums == arrSize);

						vertices.emplace_back(numBuf[0], numBuf[1], numBuf[2]);
					}
					else if (head == OBJ_HeadOfLine::def_texcoord) {
						constexpr size_t arrSize = 2;
						float numBuf[arrSize];

						const auto numOfNums = parseAllNumbersInLine(line, numBuf, arrSize);
						assert(numOfNums == arrSize);

						texcoords.emplace_back(numBuf[0], numBuf[1]);
					}
					else if (head == OBJ_HeadOfLine::def_normal) {
						constexpr size_t arrSize = 3;
						float numBuf[arrSize];

						const auto numOfNums = parseAllNumbersInLine(line, numBuf, arrSize);
						assert(numOfNums == arrSize);

						normals.emplace_back(numBuf[0], numBuf[1], numBuf[2]);
					}
					else if (head == OBJ_HeadOfLine::new_polygon) {
						constexpr size_t arrSize = 9;
						int numBuf[arrSize];

						const auto numOfNums = parseAllNumbersInLine(line, numBuf, arrSize);
						assert(numOfNums == arrSize);

						for (int i = 0; i < 3; i++) {
							const auto& vertex = vertices.at(numBuf[3 * i + 0] - 1);
							const auto & texcoord = texcoords.at(numBuf[3 * i + 1] - 1);
							const auto & normal = normals.at(numBuf[3 * i + 2] - 1);

							curObject->mVertices.push_back(vertex.x);
							curObject->mVertices.push_back(vertex.y);
							curObject->mVertices.push_back(vertex.z);
							curObject->debugVertices.push_back(vertex);

							curObject->mTexcoords.push_back(texcoord.x);
							curObject->mTexcoords.push_back(texcoord.y);
							curObject->debugTexcoords.push_back(texcoord);

							curObject->mNormals.push_back(normal.x);
							curObject->mNormals.push_back(normal.y);
							curObject->mNormals.push_back(normal.z);
							curObject->debugNormals.push_back(normal);
						}

					}
					else if (head == OBJ_HeadOfLine::def_object) {
						char strBuf[128];
						auto strSizeReturned = parseOneStrInLine(line, strBuf, 128);
						con->mObjects.emplace_back(strBuf);
						curObject = &con->mObjects.back();
					}
					else if (head == OBJ_HeadOfLine::def_material) {
						char strBuf[128];
						auto strSizeReturned = parseOneStrInLine(line, strBuf, 128);
						con->mMaterialFiles.emplace_back(strBuf);
					}
					else if (head == OBJ_HeadOfLine::use_material) {
						char strBuf[128];
						auto strSizeReturned = parseOneStrInLine(line, strBuf, 128);
						curObject->mMaterialName = strBuf;
					}
					else {
						//gLogger.putWarn("Not processed: "s + line);
					}

				}
				curIndex = 0;
			}
			else {
				bufLine[curIndex] = buf[i];
				curIndex++;
			}
		}

		if (dummyObj.mVertices.size() > 0 || dummyObj.mTexcoords.size() > 0 || dummyObj.mNormals.size() > 0) {
			gLogger.putWarn("Dummy obj has got data.");
		}

		return true;
	}

	bool parseMTL_deprecated(MTLInfo_deprecated * info, const uint8_t * const buf, const size_t bufSize) {
		std::array<char, 256> bufLine;
		unsigned int curIndex = 0;

		MTLMaterialInfo_deprecated* curMaterial = nullptr;

		for (unsigned int i = 0; i < bufSize; i++) {
			if (buf[i] == '\n') {
				bufLine.at(curIndex) = '\0';
				const char* const line = &bufLine[0];
				curIndex = 0;

				/* Precess a line */ {
					const auto head = MTL_HeadOfLine(getIndexOfMostSimilar(line, MTL_MAP, MTL_MAP_SIZE) + int(MTL_HeadOfLine::def_diffuseColor));

					if (head == MTL_HeadOfLine::def_diffuseColor) {
						if (curMaterial == nullptr) continue;

						constexpr size_t arrSize = 3;
						float numBuf[arrSize];

						const auto numOfNums = parseAllNumbersInLine(line, numBuf, arrSize);
						assert(numOfNums == arrSize);

						curMaterial->mDiffuseColor.r = numBuf[0];
						curMaterial->mDiffuseColor.g = numBuf[1];
						curMaterial->mDiffuseColor.b = numBuf[2];
					}
					else if (head == MTL_HeadOfLine::def_diffuseMap) {
						if (curMaterial == nullptr) continue;

						char strBuf[128];
						auto strSizeReturned = parseOneStrInLine(line, strBuf, 128);
						assert(128 >= strSizeReturned);
						curMaterial->mDiffuseMap = strBuf;
					}
					else if (head == MTL_HeadOfLine::def_material) {
						char strBuf[128];
						auto strSizeReturned = parseOneStrInLine(line, strBuf, 128);
						assert(128 >= strSizeReturned);
						info->mMateirals.emplace_back(strBuf);
						curMaterial = &info->mMateirals.back();
					}
					else {
						//gLogger.putWarn("Not processed: "s + line);
					}
				}
			}
			else {
				bufLine[curIndex] = buf[i];
				curIndex++;
			}
		}

		return true;
	}

}


namespace dal {

	bool parseOBJ_assimp(ModelInfo& info, const char* const assetPath) {
		info.clear();

		Assimp::Importer importer;
		importer.SetIOHandler(new AssIOSystem_Asset);
		const aiScene* const scene = importer.ReadFile(assetPath, aiProcess_Triangulate);
		//const aiScene* const scene = importer.ReadFileFromMemory(buf, bufSize, aiProcess_Triangulate | aiProcess_FlipUVs);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			LoggerGod::getinst().putError("Assimp read fail: "s + importer.GetErrorString());
			return false;
		}

		std::vector<RenderUnitInfo::MaterialInfo> materials;
		processMaterial(scene, materials);
	
		return processNode(info, materials, scene, scene->mRootNode);
	}

}