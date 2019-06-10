#pragma once

#include <string>
#include <vector>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include "p_meshStatic.h"
#include "m_collider.h"
#include "u_fileclass.h"
#include "p_uniloc.h"
#include "p_animation.h"


namespace dal {

    class IModel {

    private:
        ResourceID m_modelResID;
        AxisAlignedBoundingBox m_boundingBox;

    public:
        void setModelResID(const ResourceID& resID);
        const ResourceID& getModelResID(void) const;

        void setBoundingBox(const AxisAlignedBoundingBox& box);
        const AxisAlignedBoundingBox& getBoundingBox(void) const;

    };


    class ModelStatic : public IModel {

    private:
        struct RenderUnit {
            std::string m_meshName;
            dal::MeshStatic m_mesh;
            dal::Material m_material;
        };

        std::vector<RenderUnit> m_renderUnits;

    public:

        RenderUnit* addRenderUnit(void);

        bool isReady(void) const;

        void renderGeneral(const UnilocGeneral& uniloc, const std::list<ActorInfo>& actors) const;
        void renderDepthMap(const UnilocDepthmp& uniloc, const std::list<ActorInfo>& actors) const;

        void destroyModel(void);

    };


    class ModelAnimated : public IModel {

    private:
        struct RenderUnit {
            std::string m_meshName;
            dal::MeshAnimated m_mesh;
            dal::Material m_material;
        };

        std::vector<RenderUnit> m_renderUnits;
        SkeletonInterface m_jointInterface;
        std::vector<Animation> m_animations;
        glm::mat4 m_globalInvMat;

    public:
        Assimp::Importer m_importer;
        const aiScene* m_scene = nullptr;

    public:
        RenderUnit* addRenderUnit(void);
        void setSkeletonInterface(SkeletonInterface&& joints);
        void setAnimations(std::vector<Animation>&& animations);
        void setGlobalMat(const glm::mat4 mat);

        bool isReady(void) const;

        void renderAnimate(const UnilocAnimate& uniloc, const std::list<ActorInfo>& actors) const;
        void renderDepthMap(const UnilocDepthmp& uniloc, const std::list<ActorInfo>& actors) const;

        void destroyModel(void);

        void BoneTransform(float TimeInSeconds);
        void ReadNodeHeirarchy(float AnimationTime, const aiNode* pNode, const glm::mat4& ParentTransform);
        const aiNodeAnim* FindNodeAnim(const aiAnimation* const pAnimation, const std::string& NodeName);

        void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
        void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
        void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);

        unsigned int FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
        unsigned int FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
        unsigned int FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);

    };

}