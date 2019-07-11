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
#include "u_timer.h"


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

        void render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf, std::list<ActorInfo>& actors) const;
        void render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf, const cpnt::Transform& transform) const;
        void renderDepthMap(const UniInterfGeometry& unilocGeometry, std::list<ActorInfo>& actors) const;

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
        Timer m_animLocalTimer;

    public:
        RenderUnit* addRenderUnit(void);
        void setSkeletonInterface(SkeletonInterface&& joints);
        void setAnimations(std::vector<Animation>&& animations);
        void setGlobalMat(const glm::mat4 mat);

        bool isReady(void) const;

        void render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf,
            const UniInterfAnime& unilocAnime, std::list<ActorInfo>& actors);
        void renderDepthMap(const UniInterfGeometry& unilocGeometry, const UniInterfAnime& unilocAnime, std::list<ActorInfo>& actors) const;

        void destroyModel(void);

        void updateAnimation0(void);

        /*
        // Just in case
        void BoneTransform(float TimeInSeconds);
        void ReadNodeHeirarchy(float AnimationTime, const aiNode* pNode, const glm::mat4& ParentTransform);
        const aiNodeAnim* FindNodeAnim(const aiAnimation* const pAnimation, const std::string& NodeName);

        void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
        void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
        void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);

        unsigned int FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
        unsigned int FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
        unsigned int FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
        */

    };

}