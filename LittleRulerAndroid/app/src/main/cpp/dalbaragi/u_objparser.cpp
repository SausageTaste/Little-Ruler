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


using namespace std::string_literals;


namespace {

	auto& g_logger = dal::LoggerGod::getinst();

}


namespace {

	dal::ResourceID toAssResID(const std::string& path) {
		const auto packageSlashPos = path.find("/");
		if (std::string::npos == packageSlashPos) {
			throw "Invalid assimp res id: "s + path;
		}
		const auto package = path.substr(0, packageSlashPos);
		const auto rest = path.substr(packageSlashPos + 1, path.size() - packageSlashPos - 1);

		return dal::ResourceID{ package + "::" + rest };
	}

}


namespace {

	/*
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
			return m_handle.read(static_cast<uint8_t*>(pvBuffer), pSize * pCount);
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

		virtual Assimp::IOStream* Open(const char* pFile, const char* pMode = "rb") override {
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
	*/

	class AssResourceIOStream : public Assimp::IOStream {

	private:
		std::unique_ptr<dal::IResourceStream> m_file;
		const dal::FileMode m_mode = dal::FileMode::bread;

	public:
		AssResourceIOStream(const dal::FileMode mode)
		:	m_mode(mode)
		{

		}

		bool open(const char* const assimpResID) {
			this->m_file = dal::resopen(toAssResID(assimpResID), this->m_mode);
			return nullptr != this->m_file;
		}

		virtual size_t FileSize(void) const override {
			return this->m_file->getSize();
		}

		virtual void Flush(void) override {

		}

		virtual size_t Read(void* pvBuffer, size_t pSize, size_t pCount) override {
			return this->m_file->read(static_cast<uint8_t*>(pvBuffer), pSize * pCount);
		}

		virtual aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override {
			dal::Whence whence;

			switch (pOrigin) {

			case aiOrigin_SET:
				whence = dal::Whence::beg;
				break;
			case aiOrigin_CUR:
				whence = dal::Whence::cur;
				break;
			case aiOrigin_END:
				whence = dal::Whence::end;
				break;
			default:
				dal::LoggerGod::getinst().putError("Invalid pOrigin value for AssIOStreamAsset::Seek: "s + std::to_string(pOrigin));
				return aiReturn_FAILURE;

			}

			return this->m_file->seek(pOffset, whence) ? aiReturn_SUCCESS : aiReturn_FAILURE;
		}

		virtual size_t Tell() const override {
			return this->m_file->tell();
		}

		virtual size_t Write(const void* pvBuffer, size_t pSize, size_t pCount) override {
			const auto bufSize = pSize * pCount;
			if (this->m_file->write(static_cast<const uint8_t*>(pvBuffer), bufSize)) {
				return bufSize;
			}
			else {
				return 0;
			}
		}

	};


	class AssResourceIOSystem : public Assimp::IOSystem {

	public:
		virtual void Close(Assimp::IOStream* pFile) override {
			delete pFile;
		}

		virtual bool Exists(const char* pFile) const override {
			auto resID = toAssResID(pFile);
			return dal::filec::resolveRes(resID);
		}

		virtual char getOsSeparator() const override {
			return '/';
		}

		virtual Assimp::IOStream* Open(const char* pFile, const char* pMode = "rb") override {
			const auto mode = dal::mapFileMode(pMode);
			auto file = new AssResourceIOStream(mode);
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
			info.emplace_front();
			auto& renUnit = info.front();
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


namespace dal {

	bool parseOBJ_assimp(ModelInfo& info, ResourceID assetPath) {
		info.clear();

		Assimp::Importer importer;
		importer.SetIOHandler(new AssResourceIOSystem);
		const auto assimpResID = assetPath.getPackage() + '/' + assetPath.makeFilePath();
		const aiScene* const scene = importer.ReadFile(assimpResID.c_str(), aiProcess_Triangulate);

		//const aiScene* const scene = importer.ReadFileFromMemory(buf, bufSize, aiProcess_Triangulate | aiProcess_FlipUVs);
		if ((nullptr == scene) || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || (nullptr == scene->mRootNode)) {
			LoggerGod::getinst().putError("Assimp read fail: "s + importer.GetErrorString());
			return false;
		}

		std::vector<RenderUnitInfo::MaterialInfo> materials;
		processMaterial(scene, materials);
	
		return processNode(info, materials, scene, scene->mRootNode);
	}

}