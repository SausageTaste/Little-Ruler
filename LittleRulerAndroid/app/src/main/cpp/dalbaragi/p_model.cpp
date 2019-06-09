#include "p_model.h"

#include <glm/gtc/matrix_transform.hpp>
#include <assimp/matrix4x4.h>

#include "s_logger_god.h"

namespace {

    inline glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4* from) {
        glm::mat4 to;
        using GLfloat = float;

        to[0][0] = (GLfloat)from->a1; to[0][1] = (GLfloat)from->b1;  to[0][2] = (GLfloat)from->c1; to[0][3] = (GLfloat)from->d1;
        to[1][0] = (GLfloat)from->a2; to[1][1] = (GLfloat)from->b2;  to[1][2] = (GLfloat)from->c2; to[1][3] = (GLfloat)from->d2;
        to[2][0] = (GLfloat)from->a3; to[2][1] = (GLfloat)from->b3;  to[2][2] = (GLfloat)from->c3; to[2][3] = (GLfloat)from->d3;
        to[3][0] = (GLfloat)from->a4; to[3][1] = (GLfloat)from->b4;  to[3][2] = (GLfloat)from->c4; to[3][3] = (GLfloat)from->d4;

        return to;
    }

}


// IModel
namespace dal {

    void IModel::setModelResID(const ResourceID& resID) {
        this->m_modelResID = resID;
    }

    const ResourceID& IModel::getModelResID(void) const {
        return this->m_modelResID;
    }

    void IModel::setBoundingBox(const AxisAlignedBoundingBox& box) {
        this->m_boundingBox = box;
    }

    const AxisAlignedBoundingBox& IModel::getBoundingBox(void) const {
        return this->m_boundingBox;
    }

}


// ModelStatic
namespace dal {

    ModelStatic::RenderUnit* ModelStatic::addRenderUnit(void) {
        this->m_renderUnits.emplace_back();
        return &this->m_renderUnits.back();
    }

    bool ModelStatic::isReady(void) const {
        for ( const auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) return false;
        }

        return true;
    }

    void ModelStatic::renderGeneral(const UnilocGeneral& uniloc, const std::list<ActorInfo>& actors) const {
        if ( !this->isReady() ) return;

        for ( auto& unit : this->m_renderUnits ) {
            unit.m_material.sendUniform(uniloc);
            if ( !unit.m_mesh.isReady() ) continue;

            for ( auto& inst : actors ) {
                auto mat = inst.getViewMat();
                glUniformMatrix4fv(uniloc.uModelMat, 1, GL_FALSE, &mat[0][0]);
                unit.m_mesh.draw();
            }
        }
    }

    void ModelStatic::renderDepthMap(const UnilocDepthmp& uniloc, const std::list<ActorInfo>& actors) const {
        if ( !this->isReady() ) return;

        for ( auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) continue;

            for ( auto& inst : actors ) {
                auto mat = inst.getViewMat();
                glUniformMatrix4fv(uniloc.uModelMat, 1, GL_FALSE, &mat[0][0]);
                unit.m_mesh.draw();
            }
        }
    }

    void ModelStatic::destroyModel(void) {
        for ( auto& unit : this->m_renderUnits ) {
            unit.m_mesh.destroyData();
        }
    }

}


// ModelAnimated
namespace dal {

    ModelAnimated::RenderUnit* ModelAnimated::addRenderUnit(void) {
        this->m_renderUnits.emplace_back();
        return &this->m_renderUnits.back();
    }

    void ModelAnimated::setAnimation(const loadedinfo::JointInfoNoParent& joints) {
        this->m_jointInterface = joints;
    }

    void ModelAnimated::setGlobalMat(const glm::mat4 mat) {
        this->m_globalInvMat = glm::inverse(mat);
    }

    bool ModelAnimated::isReady(void) const {
        for ( const auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) return false;
        }

        return true;
    }

    void ModelAnimated::renderAnimate(const UnilocAnimate& uniloc, const std::list<ActorInfo>& actors) const {
        if ( !this->isReady() ) return;

        this->m_jointInterface.sendUniform(uniloc);

        for ( auto& unit : this->m_renderUnits ) {
            unit.m_material.sendUniform(uniloc);
            if ( !unit.m_mesh.isReady() ) continue;

            for ( auto& inst : actors ) {
                auto mat = inst.getViewMat();
                glUniformMatrix4fv(uniloc.uModelMat, 1, GL_FALSE, &mat[0][0]);
                unit.m_mesh.draw();
            }
        }
    }

    void ModelAnimated::renderDepthMap(const UnilocDepthmp& uniloc, const std::list<ActorInfo>& actors) const {
        if ( !this->isReady() ) return;

        for ( auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) continue;

            for ( auto& inst : actors ) {
                auto mat = inst.getViewMat();
                glUniformMatrix4fv(uniloc.uModelMat, 1, GL_FALSE, &mat[0][0]);
                unit.m_mesh.draw();
            }
        }
    }

    void ModelAnimated::destroyModel(void) {
        for ( auto& unit : this->m_renderUnits ) {
            unit.m_mesh.destroyData();
        }
    }
    

    void ModelAnimated::BoneTransform(float TimeInSeconds) {
        glm::mat4 Identity{ 1.0f };

        float TicksPerSecond = (float)(this->m_scene->mAnimations[0]->mTicksPerSecond != 0 ? this->m_scene->mAnimations[0]->mTicksPerSecond : 25.0f);
        float TimeInTicks = TimeInSeconds * TicksPerSecond;
        float AnimationTime = fmod(TimeInTicks, (float)this->m_scene->mAnimations[0]->mDuration);

        ReadNodeHeirarchy(AnimationTime, this->m_scene->mRootNode, Identity);
    }

    void ModelAnimated::ReadNodeHeirarchy(float AnimationTime, const aiNode* pNode, const glm::mat4& ParentTransform)
    {
        std::string NodeName(pNode->mName.data);

        const aiAnimation* pAnimation = this->m_scene->mAnimations[0];

        glm::mat4 NodeTransformation(aiMatrix4x4ToGlm(&pNode->mTransformation));

        const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnimation, NodeName);

        if ( nullptr != pNodeAnim ) {
            // Interpolate scaling and generate scaling transformation matrix
            aiVector3D Scaling;
            CalcInterpolatedScaling(Scaling, AnimationTime, pNodeAnim);
            const glm::vec3 scaleVec{ Scaling.x, Scaling.y, Scaling.z };
            const auto scaleMat = glm::scale(glm::mat4{ 1.0f }, scaleVec);

            // Interpolate rotation and generate rotation transformation matrix
            aiQuaternion RotationQ;
            CalcInterpolatedRotation(RotationQ, AnimationTime, pNodeAnim);
            glm::quat rotateQuat{ RotationQ.w, RotationQ.x, RotationQ.y, RotationQ.z };
            rotateQuat = glm::normalize(rotateQuat);
            const auto rotateMat = glm::mat4_cast(rotateQuat);

            // Interpolate translation and generate translation transformation matrix
            aiVector3D Translation;
            CalcInterpolatedPosition(Translation, AnimationTime, pNodeAnim);
            const glm::vec3 tranVec{ Translation.x, Translation.y, Translation.z };
            const auto posMat = glm::translate(glm::mat4{ 1.0f }, tranVec);

            // Combine the above transformations
            NodeTransformation = posMat * rotateMat * scaleMat;
        }

        glm::mat4 GlobalTransformation = ParentTransform * NodeTransformation;

        const auto jointIndex = this->m_jointInterface.getIndexOf(NodeName);
        if ( -1 != jointIndex ) {
            const auto finalTrans = this->m_globalInvMat * GlobalTransformation * this->m_jointInterface.getOffsetMat(jointIndex);
            const auto lol = glm::rotate(glm::mat4{ 1.0f }, glm::radians(-90.0f), glm::vec3{ 1, 0, 0 }) * glm::scale(glm::mat4{ 1.0f }, glm::vec3{ 0.3, 0.3, 0.3 });
            this->m_jointInterface.setFinalTransform(jointIndex, lol * finalTrans);
        }

        for ( unsigned int i = 0; i < pNode->mNumChildren; i++ ) {
            ReadNodeHeirarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation);
        }
    }

    const aiNodeAnim* ModelAnimated::FindNodeAnim(const aiAnimation* const pAnimation, const std::string& NodeName) {
        for ( unsigned int i = 0; i < pAnimation->mNumChannels; i++ ) {
            const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];

            if ( std::string(pNodeAnim->mNodeName.data) == NodeName ) {
                return pNodeAnim;
            }
        }

        return nullptr;
    }


    void ModelAnimated::CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim) {
        if ( pNodeAnim->mNumPositionKeys == 1 ) {
            Out = pNodeAnim->mPositionKeys[0].mValue;
            return;
        }

        unsigned int PositionIndex = FindPosition(AnimationTime, pNodeAnim);
        unsigned int NextPositionIndex = (PositionIndex + 1);
        dalAssert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
        float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime);
        float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
        dalAssert(Factor >= 0.0f && Factor <= 1.0f);
        const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
        const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
        aiVector3D Delta = End - Start;
        Out = Start + Factor * Delta;
    }

    void ModelAnimated::CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim) {
        // we need at least two values to interpolate...
        if ( pNodeAnim->mNumRotationKeys == 1 ) {
            Out = pNodeAnim->mRotationKeys[0].mValue;
            return;
        }

        unsigned int RotationIndex = FindRotation(AnimationTime, pNodeAnim);
        unsigned int NextRotationIndex = (RotationIndex + 1);
        dalAssert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
        float DeltaTime = (float)(pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime);
        float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
        dalAssert(Factor >= 0.0f && Factor <= 1.0f);
        const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
        const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
        aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
        Out = Out.Normalize();
    }

    void ModelAnimated::CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim) {
        if ( pNodeAnim->mNumScalingKeys == 1 ) {
            Out = pNodeAnim->mScalingKeys[0].mValue;
            return;
        }

        unsigned int ScalingIndex = FindScaling(AnimationTime, pNodeAnim);
        unsigned int NextScalingIndex = (ScalingIndex + 1);
        dalAssert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
        float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime);
        float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
        dalAssert(Factor >= 0.0f && Factor <= 1.0f);
        const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
        const aiVector3D& End = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
        aiVector3D Delta = End - Start;
        Out = Start + Factor * Delta;
    }


    unsigned int ModelAnimated::FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
    {
        for ( unsigned int i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++ ) {
            if ( AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime ) {
                return i;
            }
        }

        dalAssert(0);

        return 0;
    }

    unsigned int ModelAnimated::FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
    {
        dalAssert(pNodeAnim->mNumRotationKeys > 0);

        for ( unsigned int i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++ ) {
            if ( AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime ) {
                return i;
            }
        }

        dalAssert(0);

        return 0;
    }

    unsigned int ModelAnimated::FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim) {
        dalAssert(pNodeAnim->mNumScalingKeys > 0);

        for ( unsigned int i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++ ) {
            if ( AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime ) {
                return i;
            }
        }

        dalAssert(false);
        return 0;
    }

}