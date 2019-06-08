#include "u_objparser.h"

#include <set>
#include <map>
#include <array>
#include <string>
#include <vector>
#include <cstring>
#include <iostream>

#include <unordered_map>

#include <fmt/format.h>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/postprocess.h>

#include "s_logger_god.h"
#include "u_fileclass.h"
#include "u_timer.h"


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

    inline glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4* from) {
        glm::mat4 to;
        using GLfloat = float;

        to[0][0] = (GLfloat)from->a1; to[0][1] = (GLfloat)from->b1;  to[0][2] = (GLfloat)from->c1; to[0][3] = (GLfloat)from->d1;
        to[1][0] = (GLfloat)from->a2; to[1][1] = (GLfloat)from->b2;  to[1][2] = (GLfloat)from->c2; to[1][3] = (GLfloat)from->d2;
        to[2][0] = (GLfloat)from->a3; to[2][1] = (GLfloat)from->b3;  to[2][2] = (GLfloat)from->c3; to[2][3] = (GLfloat)from->d3;
        to[3][0] = (GLfloat)from->a4; to[3][1] = (GLfloat)from->b4;  to[3][2] = (GLfloat)from->c4; to[3][3] = (GLfloat)from->d4;

        return to;
    }

    /*
    If this function return without throwing, that means
        1. The number of positions, rotations, scales are same in all channels.
        2. The time stamps on same keyframe index are same regardless of channel.
    */
    void assertAnimationValidity(const aiAnimation *const *const animArray, unsigned int arrSize) {
        for ( unsigned int i = 0; i < arrSize; i++ ) {
            const auto anim = animArray[i];
            const auto numChannel = anim->mNumChannels;
            const auto firstChannel = anim->mChannels[0];

            const auto numKeys = firstChannel->mNumPositionKeys;
            dalAssert(numKeys == firstChannel->mNumRotationKeys);  // 1
            dalAssert(numKeys == firstChannel->mNumScalingKeys);  // 1

            for ( unsigned int j = 1; j < numKeys; j++ ) {
                const auto timestamp = firstChannel->mPositionKeys[j].mTime;
                dalAssert(timestamp == firstChannel->mRotationKeys[j].mTime);  // 2
                dalAssert(timestamp == firstChannel->mScalingKeys[j].mTime);  // 2
            }

            for ( unsigned int j = 1; j < numChannel; j++ ) {
                const aiNodeAnim* const channel = anim->mChannels[j];

                dalAssert(channel->mNumPositionKeys == channel->mNumRotationKeys);  // 1
                dalAssert(channel->mNumPositionKeys == channel->mNumScalingKeys);  // 1
                dalAssert(channel->mNumPositionKeys == numKeys);  // 1

                dalAssert(channel->mNumPositionKeys == numKeys);
                for ( unsigned int k = 0; k < numKeys; k++ ) {
                    const auto timestamp = channel->mPositionKeys[k].mTime;
                    if ( timestamp != channel->mRotationKeys[k].mTime ) {
                        dalAbort("WTF");
                    }// 2
                    dalAssert(timestamp == channel->mScalingKeys[k].mTime);  // 2
                    dalAssert(timestamp == firstChannel->mPositionKeys[k].mTime)  // 2
                }
            }
        }
    }


    struct AABBBuildInfo {
        glm::vec3 p1, p2;
    };

}  // namespace


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

    std::vector<dal::loadedinfo::Material> parseMaterials(const aiScene* const scene) {
        std::vector<dal::loadedinfo::Material> materials;

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

            aiString str;
            for ( unsigned int j = 0; j < iMaterial->GetTextureCount(aiTextureType_DIFFUSE); j++ ) {
                if ( iMaterial->GetTexture(aiTextureType_DIFFUSE, j, &str) == aiReturn_SUCCESS ) {
                    iMatInfo.m_diffuseMap = str.C_Str();
                }

                break;  // Because it supports only one diffuse map atm.
            }

            for ( unsigned int j = 0; j < iMaterial->GetTextureCount(aiTextureType_SPECULAR); j++ ) {
                if ( iMaterial->GetTexture(aiTextureType_SPECULAR, j, &str) == aiReturn_SUCCESS ) {
                    iMatInfo.m_specularMap = str.C_Str();
                }

                break;  // Because it supports only one specular map atm.
            }
        }

        return materials;
    }

    void getJointHierarchy_recur(const aiNode* const node, const dal::loadedinfo::Animation& animation, dal::JointNode& jointNode) {
        const auto nodeName = node->mName.C_Str();
        jointNode.setName(nodeName);
        jointNode.setKeyframe(animation.getJoint(nodeName));
        jointNode.setBindMat(aiMatrix4x4ToGlm(&node->mTransformation));

        for ( unsigned int i = 0; i < node->mNumChildren; i++ ) {
            const auto child = node->mChildren[i];
            if ( animation.hasJoint(child->mName.C_Str()) ) {
                getJointHierarchy_recur(child, animation, jointNode.newChild());
            }
        }
    }

    const aiNode* findRoot_recur(const aiNode* const node, const dal::loadedinfo::Animation& animation) {
        if ( animation.hasJoint(node->mName.C_Str()) ) {
            return node;
        }
        else {
            const auto numChildren = node->mNumChildren;
            for ( unsigned i = 0; i < numChildren; i++ ) {
                const auto res = findRoot_recur(node->mChildren[i], animation);
                if ( nullptr != res ) {
                    return res;
                }
            }
            return nullptr;
        }
    }

    dal::JointNode getJointHierarchy(const aiScene* const scene, const dal::loadedinfo::Animation& animation) {
        const auto rootNode = findRoot_recur(scene->mRootNode, animation);
        dalAssert(nullptr != rootNode);

        dal::JointNode rootJoint{ nullptr };
        getJointHierarchy_recur(rootNode, animation, rootJoint);
        return rootJoint;
    }

    // Returns parent map std::unordered_map< joint name, parent name >.
    void processAnimation(const aiScene* const scene, std::vector<dal::Animation>& info) {
        info.clear();

        std::vector<dal::loadedinfo::Animation> localAnimInfo;

        localAnimInfo.resize(scene->mNumAnimations);
        for ( unsigned i = 0; i < scene->mNumAnimations; i++ ) {
            const auto anim = scene->mAnimations[i];
            auto& animInfo = localAnimInfo[i];

            animInfo.m_name = anim->mName.C_Str();

            for ( unsigned j = 0; j < anim->mNumChannels; j++ ) {
                const auto channel = anim->mChannels[j];
                animInfo.m_joints.emplace_back();
                auto& jointInfo = animInfo.m_joints.back();

                jointInfo.m_name = channel->mNodeName.C_Str();

                for ( unsigned k = 0; k < channel->mNumPositionKeys; k++ ) {
                    const auto& pos = channel->mPositionKeys[k];
                    jointInfo.m_poses.emplace(pos.mTime, glm::vec3{ pos.mValue.x, pos.mValue.y, pos.mValue.z });
                }

                for ( unsigned k = 0; k < channel->mNumRotationKeys; k++ ) {
                    const auto& rot = channel->mRotationKeys[k];
                    jointInfo.m_rotates.emplace(rot.mTime, glm::quat{ rot.mValue.w, rot.mValue.x, rot.mValue.y, rot.mValue.z });
                }

                for ( unsigned k = 0; k < channel->mNumScalingKeys; k++ ) {
                    const auto& sca = channel->mScalingKeys[k];
                    const auto averageScale = (sca.mValue.x + sca.mValue.y + sca.mValue.z) / 3.0f;
                    jointInfo.m_scales.emplace(sca.mTime, averageScale);
                }
            }

            // Fill parent info
            
            info.emplace_back(anim->mName.C_Str(), std::move(getJointHierarchy(scene, animInfo)));
        }
    }

}  // namespace


// Process mesh
namespace {

    void copy3BasicVertexInfo_swapYZ(std::vector<float>& vertices, std::vector<float>& texcoords,
        std::vector<float>& normals, AABBBuildInfo& aabbInfo, const aiMesh* const mesh)
    {
        vertices.resize(mesh->mNumVertices * 3);
        texcoords.resize(mesh->mNumVertices * 2);
        normals.resize(mesh->mNumVertices * 3);

        // For aabb
        for ( unsigned int i = 0; i < mesh->mNumVertices; i++ ) {
            const auto& vertex = mesh->mVertices[i];

            vertices[3*i + 0] = vertex.x;
            vertices[3*i + 1] = vertex.z;
            vertices[3*i + 2] = vertex.y;

            if ( aabbInfo.p1.x > vertex.x )
                aabbInfo.p1.x = vertex.x;
            else if ( aabbInfo.p2.x < vertex.x )
                aabbInfo.p2.x = vertex.x;

            if ( aabbInfo.p1.y > vertex.z )
                aabbInfo.p1.y = vertex.z;
            else if ( aabbInfo.p2.y < vertex.z )
                aabbInfo.p2.y = vertex.z;

            if ( aabbInfo.p1.z > vertex.y )
                aabbInfo.p1.z = vertex.y;
            else if ( aabbInfo.p2.z < vertex.y )
                aabbInfo.p2.z = vertex.y;

            const auto& tex = mesh->mTextureCoords[0][i];
            texcoords[2 * i + 0] = tex.x;
            texcoords[2 * i + 1] = tex.y;

            const auto& normal = mesh->mNormals[i];
            normals[3*i + 0] = normal.x;
            normals[3*i + 1] = normal.z;
            normals[3*i + 2] = normal.y;
        }
    }

    void copy3BasicVertexInfo(std::vector<float>& vertices, std::vector<float>& texcoords, std::vector<float>& normals,
        AABBBuildInfo& aabbInfo, const aiMesh* const mesh)
    {
        dalAssert(sizeof(aiVector3D) == sizeof(float)*3);

        vertices.resize(mesh->mNumVertices * 3);
        texcoords.resize(mesh->mNumVertices * 2);
        normals.resize(mesh->mNumVertices * 3);

        const auto sizeFor3 = mesh->mNumVertices * 3 * sizeof(float);
        const auto sizeFor2 = mesh->mNumVertices * 2 * sizeof(float);

        memcpy_s(vertices.data(), sizeFor3, mesh->mVertices, sizeFor3);
        memcpy_s(normals.data(), sizeFor3, mesh->mNormals, sizeFor3);

        for ( unsigned int i = 0; i < mesh->mNumVertices; i++ ) {
            const auto& tex = mesh->mTextureCoords[0][i];
            texcoords[2 * i + 0] = tex.x;
            texcoords[2 * i + 1] = tex.y;
        }

        // For aabb
        for ( unsigned int i = 0; i < mesh->mNumVertices; i++ ) {
            const auto& vertex = mesh->mVertices[i];

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
    }


    bool processMesh(dal::loadedinfo::RenderUnit& renUnit, AABBBuildInfo& aabbInfo, aiMesh* const mesh, const bool swapYZ) {
        renUnit.m_name = reinterpret_cast<char*>(&mesh->mName);

        if ( swapYZ ) {
            copy3BasicVertexInfo_swapYZ(renUnit.m_mesh.m_vertices, renUnit.m_mesh.m_texcoords, renUnit.m_mesh.m_normals, aabbInfo, mesh);
        }
        else {
            copy3BasicVertexInfo(renUnit.m_mesh.m_vertices, renUnit.m_mesh.m_texcoords, renUnit.m_mesh.m_normals, aabbInfo, mesh);
        }

        return true;
    }

    bool processMeshAnimated(dal::loadedinfo::RenderUnitAnimated& renUnit, dal::loadedinfo::JointInfoNoParent& jointInfo,
        AABBBuildInfo& aabbInfo, aiMesh* const mesh, const bool swapYZ)
    {
        renUnit.m_name = reinterpret_cast<char*>(&mesh->mName);

        std::vector<std::multimap<float, unsigned int>> count;
        count.resize(mesh->mNumVertices);

        renUnit.m_mesh.m_boneWeights.resize(mesh->mNumVertices * 3);
        renUnit.m_mesh.m_boneIndex.resize(mesh->mNumVertices * 3);

        copy3BasicVertexInfo(renUnit.m_mesh.m_vertices, renUnit.m_mesh.m_texcoords, renUnit.m_mesh.m_normals, aabbInfo, mesh);
       
        for ( unsigned int i = 0; i < mesh->mNumBones; i++ ) {
            const auto bone = mesh->mBones[i];
            const auto jointIndex = jointInfo.getOrMakeIndexOf(bone->mName.C_Str());

            jointInfo.setOffsetMat(jointIndex, aiMatrix4x4ToGlm(&bone->mOffsetMatrix));

            for ( unsigned int j = 0; j < bone->mNumWeights; j++ ) {
                const auto& weight = bone->mWeights[j];
                count.at(weight.mVertexId).emplace(weight.mWeight, jointIndex);
            }
        }

        for ( size_t i = 0; i < count.size(); i++ ) {
            auto& vmap = count.at(i);

            auto iter = vmap.rbegin();
            glm::vec3 weightBuffer;
            int32_t indexBuf[3] = { -1, -1, -1 };
            for ( unsigned int j = 0; j < 3; j++ ) {
                weightBuffer[j] = iter->first;
                indexBuf[j] = iter->second;

                ++iter;
                if ( vmap.rend() == iter ) {
                    break;
                }
            }

            weightBuffer = glm::normalize(weightBuffer);
            memcpy_s(&renUnit.m_mesh.m_boneWeights[3*i], 3*sizeof(float), &weightBuffer[0], 3*sizeof(float));
            memcpy_s(&renUnit.m_mesh.m_boneIndex[3*i], 3*sizeof(int32_t), &indexBuf[0], 3*sizeof(int32_t));
        }

        return true;
    }

}  // namespace


// Process nodes
namespace {

    bool processNode(dal::loadedinfo::ModelStatic& info, const std::vector<dal::loadedinfo::Material>& materials,
        AABBBuildInfo& aabbInfo, const aiScene* const scene, const aiNode* const node, const bool swapYZ)
    {
        for ( unsigned int i = 0; i < node->mNumMeshes; i++ ) {
            aiMesh* ai_mesh = scene->mMeshes[node->mMeshes[i]];
            info.m_renderUnits.emplace_front();
            auto& renUnit = info.m_renderUnits.front();
            if ( !processMesh(renUnit, aabbInfo, ai_mesh, swapYZ) ) return false;

            if ( ai_mesh->mMaterialIndex != 0 ) {
                renUnit.m_material = materials.at(ai_mesh->mMaterialIndex);
            }
        }

        for ( unsigned int i = 0; i < node->mNumChildren; i++ ) {
            if ( processNode(info, materials, aabbInfo, scene, node->mChildren[i], swapYZ) == false ) return false;
        }

        return true;
    }

    bool processNodeAnimated(dal::loadedinfo::ModelAnimated& info, const std::vector<dal::loadedinfo::Material>& materials,
        AABBBuildInfo& aabbInfo, const aiScene* const scene, const aiNode* const node,  const bool swapYZ)
    {
        for ( unsigned int i = 0; i < node->mNumMeshes; i++ ) {
            aiMesh* ai_mesh = scene->mMeshes[node->mMeshes[i]];

            info.m_renderUnits.emplace_front();
            auto& renUnit = info.m_renderUnits.front();
            if ( !processMeshAnimated(renUnit, info.m_joints, aabbInfo, ai_mesh, swapYZ) ) return false;

            if ( ai_mesh->mMaterialIndex != 0 ) {
                renUnit.m_material = materials.at(ai_mesh->mMaterialIndex);
            }
        }

        for ( unsigned int i = 0; i < node->mNumChildren; i++ ) {
            if ( processNodeAnimated(info, materials, aabbInfo, scene, node->mChildren[i], swapYZ) == false ) return false;
        }

        return true;
    }

}  // namespace


namespace dal {

    bool loadAssimp_staticModel(loadedinfo::ModelStatic& info, const ResourceID& assetPath) {
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
        info.m_renderUnits.clear();

        bool res;
        if ( ".dae"s == assetPath.getExt() ) {
            res = processNode(info, materials, aabbInfo, scene, scene->mRootNode, true);
        }
        else {
            res = processNode(info, materials, aabbInfo, scene, scene->mRootNode, false);
        }

        info.m_aabb.set(aabbInfo.p1, aabbInfo.p2);
        return res;
    }

    bool loadAssimp_animatedModel(loadedinfo::ModelAnimated& info, std::vector<dal::loadedinfo::Animation>& anims, const ResourceID& resID) {
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

        AABBBuildInfo aabbInfo;

        dalVerbose("Start loading model: "s + resID.makeIDStr());

        bool res;
        if ( ".dae"s == resID.getExt() ) {
            res = processNodeAnimated(info, materials, aabbInfo, scene, scene->mRootNode, true);
        }
        else {
            res = processNodeAnimated(info, materials, aabbInfo, scene, scene->mRootNode, false);
        }

        info.m_aabb.set(aabbInfo.p1, aabbInfo.p2);
        return res;
    }

    bool loadAssimpModel(const ResourceID& resID, AssimpModelInfo& info, ModelAnimated& model) {
        model.m_importer.SetIOHandler(new AssResourceIOSystem);
        model.m_scene = model.m_importer.ReadFile(makeAssimpResID(resID).c_str(), aiProcess_Triangulate);
        if ( !isSceneComplete(model.m_scene) ) {
            dalError("Assimp read fail: "s +  model.m_importer.GetErrorString());
            return false;
        }

        const auto materials = parseMaterials(model.m_scene);
        processAnimation(model.m_scene, info.m_animations);

        AABBBuildInfo aabbInfo;
        info.m_model.m_globalTrans = aiMatrix4x4ToGlm(&model.m_scene->mRootNode->mTransformation);
        model.setGlobalMat(info.m_model.m_globalTrans);
        const auto res = processNodeAnimated(info.m_model, materials, aabbInfo, model.m_scene, model.m_scene->mRootNode, false);
        info.m_model.m_aabb.set(aabbInfo.p1, aabbInfo.p2);

        return true;
    }

}  // namespace dal