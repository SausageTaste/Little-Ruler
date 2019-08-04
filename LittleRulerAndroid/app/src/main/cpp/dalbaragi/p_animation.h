#pragma once

#include <map>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "p_uniloc.h"
#include "u_timer.h"


namespace dal {

    using jointID_t = int32_t;


    struct JointKeyframeInfo {
        std::string m_name;
        std::map<float, glm::vec3> m_poses;
        std::map<float, glm::quat> m_rotates;
        std::map<float, float> m_scales;
    };


    class SkeletonInterface {

    private:
        std::map<std::string, int32_t> m_map;
        std::vector<glm::mat4> m_boneOffsets;
        int32_t m_lastMadeIndex = -1;

    public:
        jointID_t getIndexOf(const std::string& jointName) const;
        jointID_t getOrMakeIndexOf(const std::string& jointName);

        void setOffsetMat(const jointID_t index, const glm::mat4& mat);
        const glm::mat4& getOffsetMat(const jointID_t index) const;

        jointID_t getSize(void) const;
        bool isEmpty(void) const;

    private:
        jointID_t upsizeAndGetIndex(void);
        bool isIndexValid(const jointID_t index) const;

    };


    class JointTransformArray {

    private:
        std::vector<glm::mat4> m_array;

    public:
        void setSize(const jointID_t size);
        void setTransform(const jointID_t index, const glm::mat4& mat);
        void sendUniform(const UniInterfAnime& uniloc) const;

        jointID_t getSize(void) const {
            return this->m_array.size();
        }

    };


    class Animation {

    public:
        class JointNode {

        private:
            std::string m_name;
            glm::mat4 m_transform;

            std::vector<std::pair<float, glm::vec3>> m_poses;
            std::vector<std::pair<float, glm::quat>> m_rotates;
            std::vector<std::pair<float, float>> m_scales;

            JointNode* m_parent;
            std::vector<JointNode> m_children;

        public:
            JointNode(const JointKeyframeInfo& info, const glm::mat4& transform, JointNode* const parent);
            JointNode(const std::string& name, const glm::mat4& transform, JointNode* const parent);

            JointNode* emplaceChild(const JointKeyframeInfo& info, const glm::mat4& transform, JointNode* const parent);
            JointNode* emplaceChild(const std::string& name, const glm::mat4& transform, JointNode* const parent);

            void sample(const float animTick, const glm::mat4& parentTrans, const SkeletonInterface& interf, const glm::mat4& globalInvMat,
                JointTransformArray& transformArr) const;

        private:
            bool hasKeyframes(void) const;

            glm::vec3 makePosInterp(const float animTick) const;
            glm::quat makeRotateInterp(const float animTick) const;
            float makeScaleInterp(const float animTick) const;
            glm::mat4 makeTransformInterp(const float animTick) const;

        };

    private:
        std::string m_name;
        JointNode m_rootNode;
        float m_tickPerSec, m_durationInTick;

    public:
        Animation(const std::string& name, const float tickPerSec, const float durationTick, JointNode&& rootNode);

        const std::string& getName(void) const { return this->m_name; }
        float getTickPerSec(void) const { return this->m_tickPerSec; }
        float getDurationInTick(void) const { return this->m_durationInTick; }

        void sample(const float animTick, const SkeletonInterface& interf, const glm::mat4& globalInvMat, JointTransformArray& transformArr) const;
        float calcAnimTick(const float seconds) const;

    };


    class AnimationState {

    private:
        Timer m_localTimer;
        JointTransformArray m_finalTransform;
        unsigned int m_selectedAnimIndex = 0;
        float m_timeScale = 1.0f;
        float m_localTimeAccumulator = 0.0f;

    public:
        float getElapsed(void) {
            const auto deltaTime = this->m_localTimer.checkGetElapsed();
            this->m_localTimeAccumulator += deltaTime * this->m_timeScale;
            return this->m_localTimeAccumulator;
        }
        JointTransformArray& getTransformArray(void) {
            return this->m_finalTransform;
        }
        unsigned int getSelectedAnimeIndex(void) const {
            return this->m_selectedAnimIndex;
        }

        void setSelectedAnimeIndex(const unsigned int index) {
            if ( this->m_selectedAnimIndex != index ) {
                this->m_selectedAnimIndex = index;
                this->m_localTimeAccumulator = 0.0f;
            }
        }
        void setTimeScale(const float scale) {
            this->m_timeScale = scale;
        }

    };


    void updateAnimeState(AnimationState& state, const std::vector<Animation>& anims, const SkeletonInterface& skeletonInterf, const glm::mat4& globalMatInv);

}