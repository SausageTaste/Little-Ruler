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


    class SkeletonInterface {

    private:
        struct BoneInfo {
            glm::mat4 m_boneOffset;
            glm::mat4 m_spaceToParent;
            jointID_t m_parentIndex = -1;
        };

    private:
        std::map<std::string, jointID_t> m_map;
        std::vector<BoneInfo> m_boneInfo;
        jointID_t m_lastMadeIndex = -1;

    public:
        SkeletonInterface(const SkeletonInterface&) = delete;
        SkeletonInterface& operator=(const SkeletonInterface&) = delete;
        SkeletonInterface(SkeletonInterface&&) = default;
        SkeletonInterface& operator=(SkeletonInterface&&) = default;

        SkeletonInterface(void) = default;

    public:
        jointID_t getIndexOf(const std::string& jointName) const;
        jointID_t getOrMakeIndexOf(const std::string& jointName);

        BoneInfo& at(const jointID_t index);
        const BoneInfo& at(const jointID_t index) const;
        const std::string& getName(const jointID_t index) const;

        jointID_t getSize(void) const;
        bool isEmpty(void) const;

        void clear(void);

    private:
        jointID_t upsizeAndGetIndex(void);
        bool isIndexValid(const jointID_t index) const;

    };


    class JointTransformArray {

    private:
        std::vector<glm::mat4> m_array;

    public:
        JointTransformArray(const JointTransformArray&) = delete;
        JointTransformArray& operator=(const JointTransformArray&) = delete;
        JointTransformArray(JointTransformArray&&) = default;
        JointTransformArray& operator=(JointTransformArray&&) = default;

        JointTransformArray(void) = default;

    public:
        void setSize(const jointID_t size);
        void setTransform(const jointID_t index, const glm::mat4& mat);
        void sendUniform(const UniInterfAnime& uniloc) const;
        void sendUniform(const UniInterf_Skeleton& uniloc) const;

        jointID_t getSize(void) const {
            return this->m_array.size();
        }

    };


    class Animation {

    public:
        class JointNode {

        private:
            std::string m_name;

            std::vector<std::pair<float, glm::vec3>> m_poses;
            std::vector<std::pair<float, glm::quat>> m_rotates;
            std::vector<std::pair<float, float>> m_scales;

        public:
            JointNode(const JointNode&) = delete;
            JointNode& operator=(const JointNode&) = delete;
            JointNode(JointNode&&) = default;
            JointNode& operator=(JointNode&&) = default;

        public:
            JointNode(void) = default;;

            void setName(const std::string& name) {
                this->m_name = name;
            }
            void addPos(const float timepoint, const glm::vec3& pos) {
                const std::pair<float, glm::vec3> input{ timepoint, pos };
                this->m_poses.push_back(input);
            }
            void addRotation(const float timepoint, const glm::quat& rot) {
                const std::pair<float, glm::quat> input{ timepoint, rot };
                this->m_rotates.push_back(input);
            }
            void addScale(const float timepoint, const float scale) {
                const std::pair<float, float> input{ timepoint, scale };
                this->m_scales.push_back(input);
            }
           
            const std::string& name(void) const {
                return this->m_name;
            }

            glm::mat4 makeTransform(const float animTick) const;

        private:
            bool hasKeyframes(void) const;

            glm::vec3 makePosInterp(const float animTick) const;
            glm::quat makeRotateInterp(const float animTick) const;
            float makeScaleInterp(const float animTick) const;

        };

    private:
        std::string m_name;
        std::vector<JointNode> m_joints;
        float m_tickPerSec, m_durationInTick;

    public:
        Animation(const Animation&) = delete;
        Animation& operator=(const Animation&) = delete;
        Animation(Animation&&) = default;
        Animation& operator=(Animation&&) = default;

    public:
        Animation(const std::string& name, const float tickPerSec, const float durationTick);

        JointNode& newJoint(void) {
            return this->m_joints.emplace_back();
        }

        const std::string& getName(void) const {
            return this->m_name;
        }
        float getTickPerSec(void) const {
            return this->m_tickPerSec;
        }
        float getDurationInTick(void) const {
            return this->m_durationInTick;
        }

        void sample2(const float animTick, const SkeletonInterface& interf, JointTransformArray& transformArr) const;
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


    void updateAnimeState(AnimationState& state, const std::vector<Animation>& anims, const SkeletonInterface& skeletonInterf);

}