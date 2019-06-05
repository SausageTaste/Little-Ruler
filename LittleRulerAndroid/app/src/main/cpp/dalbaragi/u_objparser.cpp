#include "u_objparser.h"

#include <cstring>
#include <string>
#include <array>
#include <vector>
#include <iostream>

#include <fmt/format.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>

#include "s_logger_god.h"
#include "u_fileclass.h"


using namespace std::string_literals;
using namespace fmt::literals;


// Utils
namespace {

    dal::ResourceID makeFromAssimpResID(const std::string& path) {
        const auto packageSlashPos = path.find("/");
        if ( std::string::npos == packageSlashPos ) {
            dalAbort("Invalid assimp res id: "s + path);
        }
        const auto package = path.substr(0, packageSlashPos);
        const auto rest = path.substr(packageSlashPos + 1, path.size() - packageSlashPos - 1);

        return dal::ResourceID{ package + "::" + rest };
    }

    std::string makeAssimpResID(const dal::ResourceID& resID) {
        return resID.getPackage() + '/' + resID.makeFilePath();
    }

    bool isSceneComplete(const aiScene* const scene) {
        if ( nullptr == scene ) return false;
        else if ( nullptr == scene->mRootNode ) return false;
        else if ( scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ) return false;
        else return true;
    }


    struct AABBBuildInfo {
        glm::vec3 p1, p2;
    };

    struct AninationInfo {

    };

}


// Assimp filesystem
namespace {

    class AssResourceIOStream : public Assimp::IOStream {

    private:
        std::unique_ptr<dal::IResourceStream> m_file;
        const dal::FileMode m_mode;

    public:
        AssResourceIOStream(const dal::FileMode mode)
            : m_mode(mode)
        {

        }

        bool open(const char* const assimpResID) {
            this->m_file = dal::resopen(makeFromAssimpResID(assimpResID), this->m_mode);
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

            switch ( pOrigin ) {

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
                dal::LoggerGod::getinst().putError("Invalid pOrigin value for AssIOStreamAsset::Seek: "s + std::to_string(pOrigin), __LINE__, __func__, __FILE__);
                return aiReturn_FAILURE;

            }

            return this->m_file->seek(pOffset, whence) ? aiReturn_SUCCESS : aiReturn_FAILURE;
        }

        virtual size_t Tell() const override {
            return this->m_file->tell();
        }

        virtual size_t Write(const void* pvBuffer, size_t pSize, size_t pCount) override {
            const auto bufSize = pSize * pCount;
            if ( this->m_file->write(static_cast<const uint8_t*>(pvBuffer), bufSize) ) {
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
            auto resID = makeFromAssimpResID(pFile);
            return dal::resolveRes(resID);
        }

        virtual char getOsSeparator() const override {
            return '/';
        }

        virtual Assimp::IOStream* Open(const char* pFile, const char* pMode = "rb") override {
            const auto mode = dal::mapFileMode(pMode);
            auto file = new AssResourceIOStream(mode);
            if ( file->open(pFile) ) {
                return file;
            }
            else {
                delete file;
                return nullptr;
            }

        }

    };

}


// Processing functions
namespace {

    unsigned int processMaterial(const aiScene* const scene, std::vector<dal::loadedinfo::Material>& materials) {
        for ( unsigned int i = 0; i < scene->mNumMaterials; i++ ) {
            const auto iMaterial = scene->mMaterials[i];
            materials.emplace_back();
            auto& iMatInfo = materials.back();

            {
                float floatBuf;

                if ( aiReturn_SUCCESS == aiGetMaterialFloat(iMaterial, AI_MATKEY_SHININESS, &floatBuf) ) {
                    iMatInfo.m_shininess = floatBuf;
                }

                if ( aiReturn_SUCCESS == aiGetMaterialFloat(iMaterial, AI_MATKEY_SHININESS_STRENGTH, &floatBuf) ) {
                    iMatInfo.m_specStrength = floatBuf;
                }

                aiColor4D vec4Buf;
                if ( aiReturn_SUCCESS == aiGetMaterialColor(iMaterial, AI_MATKEY_COLOR_DIFFUSE, &vec4Buf) ) {
                    iMatInfo.m_diffuseColor.r = vec4Buf.r;
                    iMatInfo.m_diffuseColor.g = vec4Buf.g;
                    iMatInfo.m_diffuseColor.b = vec4Buf.b;
                }
            }

            for ( unsigned int j = 0; j < iMaterial->GetTextureCount(aiTextureType_DIFFUSE); j++ ) {
                aiString str;
                if ( iMaterial->GetTexture(aiTextureType_DIFFUSE, j, &str) == aiReturn_SUCCESS ) {
                    iMatInfo.m_diffuseMap = str.C_Str();
                }

                break;  // Because it supports only one diffuse map atm.
            }

            for ( unsigned int j = 0; j < iMaterial->GetTextureCount(aiTextureType_SPECULAR); j++ ) {
                aiString str;
                if ( iMaterial->GetTexture(aiTextureType_SPECULAR, j, &str) == aiReturn_SUCCESS ) {
                    iMatInfo.m_specularMap = str.C_Str();
                }

                break;  // Because it supports only one specular map atm.
            }
        }

        return scene->mNumMaterials;
    }

    void processAnimation(const aiScene* const scene) {
        for ( unsigned int i = 0; i < scene->mNumAnimations; i++ ) {
            auto anim = scene->mAnimations[i];

            for ( unsigned int j = 0; j < anim->mNumChannels; j++ ) {
                auto channel = anim->mChannels[j];
                dalAssert(channel->mNumPositionKeys == channel->mNumRotationKeys);
                dalAssert(channel->mNumPositionKeys == channel->mNumScalingKeys);

                for ( unsigned int k = 0; k < channel->mNumPositionKeys; k++ ) {
                    auto pos = channel->mPositionKeys[k];
                    auto rot = channel->mRotationKeys[k];
                    auto sca = channel->mScalingKeys[k];
                    dalAssert(pos.mTime == rot.mTime);
                    dalAssert(pos.mTime == sca.mTime);
                }
            }
        }
    }

    bool processMesh(dal::loadedinfo::RenderUnit& renUnit, AABBBuildInfo& aabbInfo, aiMesh* const mesh) {
        renUnit.m_name = reinterpret_cast<char*>(&mesh->mName);

        renUnit.m_mesh.m_vertices.reserve(mesh->mNumVertices * 3);
        renUnit.m_mesh.m_texcoords.reserve(mesh->mNumVertices * 2);
        renUnit.m_mesh.m_normals.reserve(mesh->mNumVertices * 3);

        for ( unsigned int i = 0; i < mesh->mNumVertices; i++ ) {
            auto vertex = mesh->mVertices[i];
            renUnit.m_mesh.m_vertices.push_back(vertex.x);
            renUnit.m_mesh.m_vertices.push_back(vertex.y);
            renUnit.m_mesh.m_vertices.push_back(vertex.z);

            {
                if ( aabbInfo.p1.x > vertex.x )
                    aabbInfo.p1.x = vertex.x;
                else if ( aabbInfo.p2.x < vertex.x )
                    aabbInfo.p2.x = vertex.x;

                if ( aabbInfo.p1.y > vertex.y )
                    aabbInfo.p1.y = vertex.y;
                else if ( aabbInfo.p2.y < vertex.y )
                    aabbInfo.p2.y = vertex.y;

                if ( aabbInfo.p1.z > vertex.z )
                    aabbInfo.p1.z = vertex.z;
                else if ( aabbInfo.p2.z < vertex.z )
                    aabbInfo.p2.z = vertex.z;
            }

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

    bool processMesh(dal::loadedinfo::RenderUnitAnimated& renUnit, AABBBuildInfo& aabbInfo, aiMesh* const mesh) {
        renUnit.m_name = reinterpret_cast<char*>(&mesh->mName);

        renUnit.m_mesh.m_vertices.reserve(mesh->mNumVertices * 3);
        renUnit.m_mesh.m_texcoords.reserve(mesh->mNumVertices * 2);
        renUnit.m_mesh.m_normals.reserve(mesh->mNumVertices * 3);
        renUnit.m_mesh.m_boneWeights.reserve(mesh->mNumVertices * 3);
        renUnit.m_mesh.m_boneIndex.reserve(mesh->mNumVertices * 3);

        for ( unsigned int i = 0; i < mesh->mNumVertices; i++ ) {
            auto vertex = mesh->mVertices[i];
            renUnit.m_mesh.m_vertices.push_back(vertex.x);
            renUnit.m_mesh.m_vertices.push_back(vertex.y);
            renUnit.m_mesh.m_vertices.push_back(vertex.z);

            {
                if ( aabbInfo.p1.x > vertex.x )
                    aabbInfo.p1.x = vertex.x;
                else if ( aabbInfo.p2.x < vertex.x )
                    aabbInfo.p2.x = vertex.x;

                if ( aabbInfo.p1.y > vertex.y )
                    aabbInfo.p1.y = vertex.y;
                else if ( aabbInfo.p2.y < vertex.y )
                    aabbInfo.p2.y = vertex.y;

                if ( aabbInfo.p1.z > vertex.z )
                    aabbInfo.p1.z = vertex.z;
                else if ( aabbInfo.p2.z < vertex.z )
                    aabbInfo.p2.z = vertex.z;
            }

            auto texCoord = mesh->mTextureCoords[0][i];
            renUnit.m_mesh.m_texcoords.push_back(texCoord.x);
            renUnit.m_mesh.m_texcoords.push_back(texCoord.y);

            auto normal = mesh->mNormals[i];
            renUnit.m_mesh.m_normals.push_back(normal.x);
            renUnit.m_mesh.m_normals.push_back(normal.y);
            renUnit.m_mesh.m_normals.push_back(normal.z);
        }

        for ( unsigned int i = 0; i < mesh->mNumBones; i++ ) {
            auto bone = mesh->mBones[i];

        }

        return true;
    }

    bool processNode(dal::loadedinfo::ModelStatic& info, const std::vector<dal::loadedinfo::Material>& materials, AABBBuildInfo& aabbInfo, const aiScene* const scene, const aiNode* const node) {
        for ( unsigned int i = 0; i < node->mNumMeshes; i++ ) {
            aiMesh* ai_mesh = scene->mMeshes[node->mMeshes[i]];
            info.m_renderUnits.emplace_front();
            auto& renUnit = info.m_renderUnits.front();
            if ( !processMesh(renUnit, aabbInfo, ai_mesh) ) return false;

            if ( ai_mesh->mMaterialIndex != 0 ) {
                renUnit.m_material = materials.at(ai_mesh->mMaterialIndex);
            }
        }

        for ( unsigned int i = 0; i < node->mNumChildren; i++ ) {
            if ( processNode(info, materials, aabbInfo, scene, node->mChildren[i]) == false ) return false;
        }

        return true;
    }

    bool processNode(dal::loadedinfo::ModelAnimated& info, const std::vector<dal::loadedinfo::Material>& materials, AABBBuildInfo& aabbInfo, const aiScene* const scene, const aiNode* const node) {
        for ( unsigned int i = 0; i < node->mNumMeshes; i++ ) {
            aiMesh* ai_mesh = scene->mMeshes[node->mMeshes[i]];
            info.m_renderUnits.emplace_front();
            auto& renUnit = info.m_renderUnits.front();
            if ( !processMesh(renUnit, aabbInfo, ai_mesh) ) return false;

            if ( ai_mesh->mMaterialIndex != 0 ) {
                renUnit.m_material = materials.at(ai_mesh->mMaterialIndex);
            }
        }

        for ( unsigned int i = 0; i < node->mNumChildren; i++ ) {
            if ( processNode(info, materials, aabbInfo, scene, node->mChildren[i]) == false ) return false;
        }

        return true;
    }

}


namespace dal {

    bool loadAssimp_staticModel(loadedinfo::ModelStatic& info, ResourceID assetPath) {
        info.m_renderUnits.clear();

        Assimp::Importer importer;
        importer.SetIOHandler(new AssResourceIOSystem);
        const auto scene = importer.ReadFile(makeAssimpResID(assetPath).c_str(), aiProcess_Triangulate);
        if ( !isSceneComplete(scene) ) {
            dalError("Assimp read fail: "s + importer.GetErrorString());
            return false;
        }

        std::vector<loadedinfo::Material> materials;
        processMaterial(scene, materials);

        AABBBuildInfo aabbInfo;
        const auto res = processNode(info, materials, aabbInfo, scene, scene->mRootNode);
        info.m_aabb.set(aabbInfo.p1, aabbInfo.p2);
        return res;
    }

    bool loadAssimp_animatedModel(loadedinfo::ModelAnimated& info, ResourceID resID) {
        info.m_renderUnits.clear();
       
        Assimp::Importer importer;
        importer.SetIOHandler(new AssResourceIOSystem);
        const auto scene = importer.ReadFile(makeAssimpResID(resID).c_str(), aiProcess_Triangulate);
        if ( !isSceneComplete(scene) ) {
            dalError("Assimp read fail: "s + importer.GetErrorString());
            return false;
        }

        std::vector<loadedinfo::Material> materials;
        processMaterial(scene, materials);

        processAnimation(scene);

        AABBBuildInfo aabbInfo;
        const auto res = processNode(info, materials, aabbInfo, scene, scene->mRootNode);
        info.m_aabb.set(aabbInfo.p1, aabbInfo.p2);
        return res;
    }

}