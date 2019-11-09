#include "u_objparser.h"

#include <unordered_map>

#include <glm/gtc/matrix_transform.hpp>
#include <fmt/format.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/postprocess.h>

#include "s_logger_god.h"
#include "u_timer.h"
#include "u_byteutils.h"


using namespace fmt::literals;


// Utils
namespace {

    dal::ResourceID makeFromAssimpResID(const std::string& path) {
        const auto packageSlashPos = path.find("/");
        if ( std::string::npos == packageSlashPos ) {
            dalAbort("Invalid assimp res id: {}"_format(path));
        }
        const auto package = path.substr(0, packageSlashPos);
        const auto rest = path.substr(packageSlashPos + 1, path.size() - packageSlashPos - 1);

        return dal::ResourceID{ package + "::" + rest };
    }

    std::string makeAssimpResID(const dal::ResourceID& resID) {
        return resID.getPackage() + '/' + resID.makeFilePath();
    }

    bool isSceneComplete(const aiScene* const scene) {
        if ( nullptr == scene )
            return false;
        else if ( nullptr == scene->mRootNode )
            return false;
        else if ( scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE )
            return false;
        else
            return true;
    }

    inline glm::mat4 convertAssimpMat(const aiMatrix4x4& from) {
        glm::mat4 to;

        to[0][0] = from.a1; to[0][1] = from.b1;  to[0][2] = from.c1; to[0][3] = from.d1;
        to[1][0] = from.a2; to[1][1] = from.b2;  to[1][2] = from.c2; to[1][3] = from.d2;
        to[2][0] = from.a3; to[2][1] = from.b3;  to[2][2] = from.c3; to[2][3] = from.d3;
        to[3][0] = from.a4; to[3][1] = from.b4;  to[3][2] = from.c4; to[3][3] = from.d4;

        return to;
    }

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
                dalError("Invalid pOrigin value for AssIOStreamAsset::Seek: {}"_format(pOrigin));
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

    std::vector<dal::binfo::Material> parseMaterials(const aiScene* const scene) {
        std::vector<dal::binfo::Material> materials;
        materials.resize(scene->mNumMaterials);

        for ( unsigned int i = 0; i < scene->mNumMaterials; i++ ) {
            const auto assMaterial = scene->mMaterials[i];
            auto& matInfo = materials[i];

            {
                float floatBuf;

                if ( aiReturn_SUCCESS == aiGetMaterialFloat(assMaterial, AI_MATKEY_SHININESS, &floatBuf) ) {

                }

                if ( aiReturn_SUCCESS == aiGetMaterialFloat(assMaterial, AI_MATKEY_SHININESS_STRENGTH, &floatBuf) ) {

                }
            }

            aiString str;

            if ( assMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0 ) {
                if ( assMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &str) == aiReturn_SUCCESS ) {
                    matInfo.m_diffuseMap = str.C_Str();
                }
            }
        }

        return materials;
    }

}


// Process animations
namespace {

    using JointRegistry = std::unordered_map<std::string, dal::JointKeyframeInfo>;


    const dal::JointKeyframeInfo* findJointRegistry(const std::string& nameToFind, const JointRegistry& jointRegistry) {
        auto iter = jointRegistry.find(nameToFind);
        if ( jointRegistry.end() != iter ) {
            return &(iter->second);
        }
        else {
            return nullptr;
        }
    }

    void buildNodesRecur(const aiNode* const node, dal::Animation::JointNode& parentJoint, const JointRegistry& jointRegistry) {
        const auto thisNodeName = node->mName.C_Str();
        const auto keyframeInfo = findJointRegistry(thisNodeName, jointRegistry);

        auto rootNode = nullptr != keyframeInfo ?
            parentJoint.emplaceChild(*keyframeInfo, convertAssimpMat(node->mTransformation), &parentJoint) :
            parentJoint.emplaceChild(thisNodeName, convertAssimpMat(node->mTransformation), &parentJoint);

        for ( unsigned int i = 0; i < node->mNumChildren; i++ ) {
            buildNodesRecur(node->mChildren[i], *rootNode, jointRegistry);
        }
    }

    dal::Animation::JointNode makeJointHierarchy(const aiNode* const node, const JointRegistry& jointRegistry) {
        const auto thisNodeName = node->mName.C_Str();
        const auto keyframeInfo = findJointRegistry(thisNodeName, jointRegistry);

        auto rootNode = nullptr != keyframeInfo ?
            dal::Animation::JointNode{ *keyframeInfo, convertAssimpMat(node->mTransformation), nullptr } :
            dal::Animation::JointNode{ thisNodeName, convertAssimpMat(node->mTransformation), nullptr };

        for ( unsigned int i = 0; i < node->mNumChildren; i++ ) {
            buildNodesRecur(node->mChildren[i], rootNode, jointRegistry);
        }

        return rootNode;
    }

    void processAnimation(const aiScene* const scene, std::vector<dal::Animation>& info) {
        info.clear();

        {
            info.emplace_back("nullpose", 0.0f, 0.0f,
                makeJointHierarchy(scene->mRootNode, JointRegistry{})
            );
        }

        for ( unsigned i = 0; i < scene->mNumAnimations; i++ ) {
            const auto anim = scene->mAnimations[i];
            std::unordered_map<std::string, dal::JointKeyframeInfo> jointRegistry;
            jointRegistry.reserve(anim->mNumChannels);

            for ( unsigned j = 0; j < anim->mNumChannels; j++ ) {
                const auto channel = anim->mChannels[j];
                auto iter = jointRegistry.emplace(channel->mNodeName.C_Str(), dal::JointKeyframeInfo{});
                auto jointInfo = &(iter.first->second);

                jointInfo->m_name = channel->mNodeName.C_Str();

                for ( unsigned k = 0; k < channel->mNumPositionKeys; k++ ) {
                    const auto& pos = channel->mPositionKeys[k];
                    jointInfo->m_poses.emplace(static_cast<float>(pos.mTime), glm::vec3{ pos.mValue.x, pos.mValue.y, pos.mValue.z });
                }

                for ( unsigned k = 0; k < channel->mNumRotationKeys; k++ ) {
                    const auto& rot = channel->mRotationKeys[k];
                    jointInfo->m_rotates.emplace(static_cast<float>(rot.mTime), glm::quat{ rot.mValue.w, rot.mValue.x, rot.mValue.y, rot.mValue.z });
                }

                for ( unsigned k = 0; k < channel->mNumScalingKeys; k++ ) {
                    const auto& sca = channel->mScalingKeys[k];
                    const auto averageScale = (sca.mValue.x + sca.mValue.y + sca.mValue.z) / 3.0f;
                    jointInfo->m_scales.emplace(static_cast<float>(sca.mTime), averageScale);
                }
            }

            // Fill parent info

            info.emplace_back(
                anim->mName.C_Str(),
                static_cast<float>(anim->mTicksPerSecond),
                static_cast<float>(anim->mDuration),
                makeJointHierarchy(scene->mRootNode, jointRegistry)
            );
        }
    }

}


// Process mesh
namespace {

    void updateAABB(const aiMesh* const mesh, dal::AABB& aabb) {
        for ( unsigned int i = 0; i < mesh->mNumVertices; i++ ) {
            const auto& v = mesh->mVertices[i];
            aabb.resizeToInclude(v.x, v.y, v.z);
        }
    }

    // I don't know if big endian systems cannot use this algorithm.
    void copy3BasicVertexInfo(std::vector<float>& vertices, std::vector<float>& texcoords, std::vector<float>& normals,
        const aiMesh* const mesh, dal::ColTriangleSoup& detailed)
    {
        static_assert(sizeof(aiVector3D) == sizeof(float) * 3);

        vertices.resize(static_cast<size_t>(mesh->mNumVertices) * 3);
        texcoords.resize(static_cast<size_t>(mesh->mNumVertices) * 2);
        normals.resize(static_cast<size_t>(mesh->mNumVertices) * 3);

        const auto sizeFor3 = static_cast<size_t>(mesh->mNumVertices) * 3 * sizeof(float);
        std::memcpy(vertices.data(), mesh->mVertices, sizeFor3);
        std::memcpy(normals.data(), mesh->mNormals, sizeFor3);

        for ( size_t i = 0; i < mesh->mNumVertices; i++ ) {
            const auto& tex = mesh->mTextureCoords[0][i];
            texcoords[2 * i + 0] = tex.x;
            texcoords[2 * i + 1] = tex.y;
        }

        const auto numTriangles = static_cast<size_t>(mesh->mNumVertices) / 3;
        /*
        detailed.reserve(numTriangles);
        for ( size_t i = 0; i < numTriangles; ++i ) {
            const glm::vec3 p1{ vertices[9 * i + 0], vertices[9 * i + 1], vertices[9 * i + 2] };
            const glm::vec3 p2{ vertices[9 * i + 3], vertices[9 * i + 4], vertices[9 * i + 5] };
            const glm::vec3 p3{ vertices[9 * i + 6], vertices[9 * i + 7], vertices[9 * i + 8] };
            detailed.emplaceTriangle(p1, p2, p3);
        }
        /*/
        const auto sizeBefore = detailed.getSize();
        detailed.resize(sizeBefore + numTriangles);
        std::memcpy(detailed.data() + sizeBefore, mesh->mVertices, numTriangles * sizeof(dal::Triangle));
        //*/
    }

    bool processMeshAnimated(dal::binfo::RenderUnit& renUnit, dal::SkeletonInterface& jointInfo,
        dal::AABB& aabb, aiMesh* const mesh, dal::ColTriangleSoup& detailed)
    {
        renUnit.m_name = reinterpret_cast<char*>(&mesh->mName);
        copy3BasicVertexInfo(renUnit.m_mesh.m_vertices, renUnit.m_mesh.m_texcoords, renUnit.m_mesh.m_normals, mesh, detailed);
        updateAABB(mesh, aabb);

        if ( mesh->mNumBones > 0 ) {
            size_t numVert = static_cast<size_t>(mesh->mNumVertices);
            renUnit.m_mesh.m_boneWeights.resize(numVert * 3U);
            renUnit.m_mesh.m_boneIndex.resize(numVert * 3U);

            std::vector<std::multimap<float, unsigned int>> count;
            count.resize(mesh->mNumVertices);

            for ( unsigned int i = 0; i < mesh->mNumBones; i++ ) {
                const auto bone = mesh->mBones[i];
                const auto jointIndex = jointInfo.getOrMakeIndexOf(bone->mName.C_Str());

                jointInfo.at(jointIndex).m_boneOffset = convertAssimpMat(bone->mOffsetMatrix);

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
                    if ( vmap.rend() == iter ) {
                        break;
                    }

                    weightBuffer[j] = iter->first;
                    indexBuf[j] = iter->second;

                    ++iter;
                }

                weightBuffer = glm::normalize(weightBuffer);

                std::memcpy(&renUnit.m_mesh.m_boneWeights[3 * i], &weightBuffer[0], 3 * sizeof(float));
                std::memcpy(&renUnit.m_mesh.m_boneIndex[3 * i], &indexBuf[0], 3 * sizeof(int32_t));
            }
        }

        return true;
    }

}


// Process nodes
namespace {

    bool processNodeAnimated(dal::binfo::Model& info, const std::vector<dal::binfo::Material>& materials,
        dal::AABB& aabbInfo, const aiScene* const scene, const aiNode* const node, dal::ColTriangleSoup& detailed)
    {
        info.m_renderUnits.reserve(node->mNumMeshes);
        for ( unsigned int i = 0; i < node->mNumMeshes; i++ ) {
            aiMesh* ai_mesh = scene->mMeshes[node->mMeshes[i]];
            auto& renUnit = info.m_renderUnits.emplace_back();
            if ( !processMeshAnimated(renUnit, info.m_joints, aabbInfo, ai_mesh, detailed) ) {
                return false;
            }
            renUnit.m_material = materials.at(ai_mesh->mMaterialIndex);
        }

        for ( unsigned int i = 0; i < node->mNumChildren; i++ ) {
            if ( !processNodeAnimated(info, materials, aabbInfo, scene, node->mChildren[i], detailed) ) {
                return false;
            }
        }

        return true;
    }

}


// DMD
namespace {

    void makeAnimJoint_recur(dal::Animation::JointNode& parent, const dal::jointID_t parIndex, const dal::SkeletonInterface& skeleton, std::vector<dal::Animation::JointNode*>& resultList) {
        const auto numJoints = skeleton.getSize();

        size_t numChildren = 0;
        for ( int i = 0; i < numJoints; ++i ) {
            if ( skeleton.at(i).m_parentIndex == parIndex ) {
                numChildren++;
            }
        }
        parent.reserveChildrenVector(numChildren);

        for ( int i = 0; i < numJoints; ++i ) {
            if ( skeleton.at(i).m_parentIndex != parIndex ) {
                continue;
            }

            auto child = parent.emplaceChild(skeleton.getName(i), glm::mat4{ 1.f }, &parent);
            resultList[i] = child;
            makeAnimJoint_recur(*child, i, skeleton, resultList);
        }
    }

    auto makeAnimJointHierarchy(const dal::SkeletonInterface& skeleton) {
        std::pair<dal::Animation::JointNode, std::vector<dal::Animation::JointNode*>> result;

        const auto numJoints = skeleton.getSize();
        result.second.resize(numJoints);
        for ( auto& x : result.second ) {
            x = nullptr;
        }

        dalAssert(-1 == skeleton.at(0).m_parentIndex);

        result.first.setName(skeleton.getName(0));
        result.second[0] = &result.first;  // The address of root node changes upon return.

        makeAnimJoint_recur(result.first, 0, skeleton, result.second);

        for ( const auto x : result.second ) {
            dalAssert(nullptr != x);
        }

        return result;
    }


    const uint8_t* parse_aabb(const uint8_t* header, const uint8_t* const end, dal::AABB& aabb) {
        float fbuf[6];
        header = dal::assemble4BytesArray<float>(header, fbuf, 6);

        aabb.set(glm::vec3{ fbuf[0], fbuf[1], fbuf[2] }, glm::vec3{ fbuf[3], fbuf[4], fbuf[5] });

        return header;
    }

    const uint8_t* parse_mat4(const uint8_t* header, const uint8_t* const end, glm::mat4& mat) {
        float fbuf[16];
        header = dal::assemble4BytesArray<float>(header, fbuf, 16);

        for ( size_t row = 0; row < 4; ++row ) {
            for ( size_t col = 0; col < 4; ++col ) {
                mat[col][row] = fbuf[4 * row + col];
            }
        }

        return header;
    }

    const uint8_t* parse_skeleton(const uint8_t* header, const uint8_t* const end, dal::SkeletonInterface& skeleton) {
        const auto numBones = dal::makeInt4(header); header += 4;

        for ( int i = 0; i < numBones; ++i ) {
            const std::string boneName = reinterpret_cast<const char*>(header);
            header += boneName.size() + 1;
            const auto parentIndex = dal::makeInt4(header); header += 4;
            

            const auto result = skeleton.getOrMakeIndexOf(boneName);
            dalAssert(result == i);
            skeleton.at(result).m_parentIndex = parentIndex;
            header = parse_mat4(header, end, skeleton.at(result).m_boneOffset);
        }

        return header;
    }

    const uint8_t* parse_animJoint(const uint8_t* header, const uint8_t* const end, dal::Animation::JointNode& joint) {
        {
            glm::mat4 mat;
            header = parse_mat4(header, end, mat);
            joint.setMat(mat);
        }

        {
            const auto num = dal::makeInt4(header); header += 4;
            for ( int i = 0; i < num; ++i ) {
                float fbuf[4];
                header = dal::assemble4BytesArray<float>(header, fbuf, 4);
                joint.addPos(fbuf[0], glm::vec3{ fbuf[1], fbuf[2], fbuf[3] });
            }
        }

        {
            const auto num = dal::makeInt4(header); header += 4;
            for ( int i = 0; i < num; ++i ) {
                float fbuf[5];
                header = dal::assemble4BytesArray<float>(header, fbuf, 5);
                joint.addRotation(fbuf[0], glm::quat{ fbuf[4], fbuf[1], fbuf[2], fbuf[3] });
            }
        }

        {
            const auto num = dal::makeInt4(header); header += 4;
            for ( int i = 0; i < num; ++i ) {
                float fbuf[2];
                header = dal::assemble4BytesArray<float>(header, fbuf, 2);
                joint.addScale(fbuf[0], fbuf[1]);
            }
        }

        return header;
    }

    const uint8_t* parse_animations(const uint8_t* header, const uint8_t* const end, const dal::SkeletonInterface& skeleton, std::vector<dal::Animation>& animations) {
        const auto numAnims = dal::makeInt4(header); header += 4;

        if ( numAnims > 0 ) {
            auto [rootNode, jointList] = makeAnimJointHierarchy(skeleton);
            animations.emplace_back("nullptr", 0.f, 0.f, std::move(rootNode));
        }

        for ( int i = 0; i < numAnims; ++i ) {
            const std::string animName = reinterpret_cast<const char*>(header);
            header += animName.size() + 1;

            const auto durationTick = dal::assemble4Bytes<float>(header); header += 4;
            const auto tickPerSec = dal::assemble4Bytes<float>(header); header += 4;

            const auto numJoints = dal::makeInt4(header); header += 4;

            auto [rootNode, jointList] = makeAnimJointHierarchy(skeleton);
            jointList[0] = &rootNode;

            for ( int j = 0; j < numJoints; ++j ) {
                header = parse_animJoint(header, end, *jointList[j]);
            }
            jointList.clear();

            animations.emplace_back(animName, tickPerSec, durationTick, std::move(rootNode));
        }

        return header;
    }

    const uint8_t* parse_material(const uint8_t* const begin, const uint8_t* const end, dal::binfo::Material& material) {
        const uint8_t* header = begin;

        material.m_roughness = dal::assemble4Bytes<float>(header); header += 4;
        material.m_metallic = dal::assemble4Bytes<float>(header); header += 4;

        material.m_diffuseMap = reinterpret_cast<const char*>(header);
        header += material.m_diffuseMap.size() + 1;

        material.m_roughnessMap = reinterpret_cast<const char*>(header);
        header += material.m_roughnessMap.size() + 1;

        material.m_metallicMap = reinterpret_cast<const char*>(header);
        header += material.m_metallicMap.size() + 1;

        return header;
    }

    const uint8_t* parse_renderUnit(const uint8_t* const begin, const uint8_t* const end, dal::binfo::RenderUnit& unit) {
        const uint8_t* header = begin;

        // Name
        {
            unit.m_name = reinterpret_cast<const char*>(header);
            header += unit.m_name.size() + 1;
        }

        // Material
        {
            header = parse_material(header, end, unit.m_material);
        }

        // Vertices
        {
            const auto numVert = dal::makeInt4(header); header += 4;
            const auto hasBones = dal::makeBool1(header); header += 1;
            const auto numVertTimes3 = numVert * 3;
            const auto numVertTimes2 = numVert * 2;

            unit.m_mesh.m_vertices.resize(numVertTimes3);
            header = dal::assemble4BytesArray<float>(header, unit.m_mesh.m_vertices.data(), numVertTimes3);

            unit.m_mesh.m_texcoords.resize(numVertTimes2);
            header = dal::assemble4BytesArray<float>(header, unit.m_mesh.m_texcoords.data(), numVertTimes2);

            unit.m_mesh.m_normals.resize(numVertTimes3);
            header = dal::assemble4BytesArray<float>(header, unit.m_mesh.m_normals.data(), numVertTimes3);

            if ( hasBones ) {
                unit.m_mesh.m_boneWeights.resize(numVertTimes3);
                header = dal::assemble4BytesArray<float>(header, unit.m_mesh.m_boneWeights.data(), numVertTimes3);

                unit.m_mesh.m_boneIndex.resize(numVertTimes3);
                header = dal::assemble4BytesArray<int32_t>(header, unit.m_mesh.m_boneIndex.data(), numVertTimes3);
            }
        }

        return header;
    }

}


namespace dal {

    bool loadAssimpModel(const ResourceID& resID, AssimpModelInfo& info) {
        Assimp::Importer importer;
        importer.SetIOHandler(new AssResourceIOSystem);
        const auto scene = importer.ReadFile(makeAssimpResID(resID).c_str(), aiProcess_Triangulate);
        if ( !isSceneComplete(scene) ) {
            dalError("Assimp read fail: {}"_format(importer.GetErrorString()));
            return false;
        }

        const auto materials = parseMaterials(scene);
        info.m_model.m_globalTrans = convertAssimpMat(scene->mRootNode->mTransformation);

        dal::AABB aabbInfo;
        auto soup = new dal::ColTriangleSoup;
        info.m_detailedCol.reset(soup);
        const auto res = processNodeAnimated(info.m_model, materials, info.m_model.m_aabb, scene, scene->mRootNode, *soup);
        //TODO : Error handling.

        if ( info.m_model.m_joints.getSize() > 0 ) {
            processAnimation(scene, info.m_animations);
        }

        return true;
    }

    bool loadDalModel(const ResourceID& resID, AssimpModelInfo& info) {
        std::vector<uint8_t> filebuf;
        if ( !dal::futil::getRes_buffer(resID, filebuf) ) {
            return false;
        }
        const auto fullSize = dal::makeInt4(filebuf.data());
        std::vector<uint8_t> unzipped;
        unzipped.resize(fullSize);
        const auto unzipSize = dal::unzip(unzipped.data(), unzipped.size(), filebuf.data() + 4, filebuf.size() - 4);
        if ( 0 == unzipSize ) {
            return false;
        }

        // Start parsing
        {
            const uint8_t* const end = unzipped.data() + unzipped.size();
            const uint8_t* header = unzipped.data();

            header = parse_aabb(header, end, info.m_model.m_aabb);
            header = parse_skeleton(header, end, info.m_model.m_joints);
            header = parse_animations(header, end, info.m_model.m_joints, info.m_animations);

            const auto numUnits = dal::makeInt4(header); header += 4;
            for ( int i = 0; i < numUnits; ++i ) {
                auto& unit = info.m_model.m_renderUnits.emplace_back();
                header = parse_renderUnit(header, end, unit);
            }

            dalAssert(header == end);
        }

        return true;
    }

}
