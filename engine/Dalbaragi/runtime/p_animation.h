#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <daltools/scene/struct.h>

#include "p_uniloc.h"
#include "u_timer.h"


namespace dal {

    using jointID_t = int32_t;

    using JointType = dal::parser::JointType;


    class JointInfo {

    private:
        std::string m_name;
        glm::mat4 m_jointOffset;
        glm::mat4 m_jointOffsetInv;
        glm::mat4 m_spaceToParent;
        jointID_t m_parentIndex = -1;
        JointType m_jointType = JointType::basic;

    public:
        const std::string& name(void) const {
            return this->m_name;
        }
        const glm::mat4& offset(void) const {
            return this->m_jointOffset;
        }
        const glm::mat4& offsetInv(void) const {
            return this->m_jointOffsetInv;
        }
        const glm::mat4& toParent(void) const {
            return this->m_spaceToParent;
        }
        jointID_t parentIndex(void) const {
            return this->m_parentIndex;
        }
        JointType jointType(void) const {
            return this->m_jointType;
        }

        glm::vec3 localPos(void) const;

        void setName(const std::string& name) {
            this->m_name = name;
        }
        void setName(std::string&& name) {
            this->m_name = std::move(name);
        }
        void setOffset(const glm::mat4& mat);
        void setParentMat(const glm::mat4& mat) {
            this->m_spaceToParent = mat;
        }
        void setParentMat(const JointInfo& parent) {
            this->setParentMat(parent.offsetInv() * this->offset());
        }
        void setParentIndex(const jointID_t id) {
            this->m_parentIndex = id;
        }
        void setType(const JointType type) {
            this->m_jointType = type;
        }

    };

    class SkeletonInterface {

    private:
        std::map<std::string, jointID_t> m_map;
        std::vector<JointInfo> m_boneInfo;
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

        JointInfo& at(const jointID_t index);
        const JointInfo& at(const jointID_t index) const;

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
        void sendUniform(const UniInterf_Skeleton& uniloc) const;

        jointID_t getSize(void) const {
            return this->m_array.size();
        }

    };


    class IJointModifier {

    public:
        virtual ~IJointModifier(void) = default;
        virtual glm::mat4 makeTransform(const float deltaTime, const jointID_t jid, const dal::SkeletonInterface& skeleton) = 0;

    };

    using jointModifierRegistry_t = std::unordered_map<jointID_t, std::shared_ptr<IJointModifier>>;


    class Animation {

    public:
        class JointNode {

        private:
            dal::parser::AnimJoint m_data;

        public:
            JointNode(const JointNode&) = delete;
            JointNode& operator=(const JointNode&) = delete;
            JointNode(JointNode&&) = default;
            JointNode& operator=(JointNode&&) = default;

        public:
            JointNode() = default;

            void set(const dal::parser::AnimJoint& data) {
                this->m_data = data;
            }

            void setName(const std::string& name) {
                this->m_data.name_ = name;
            }

            void addPos(const float timepoint, const glm::vec3& pos) {
                this->m_data.add_position(timepoint, pos.x, pos.y, pos.z);
            }

            void addRotation(const float timepoint, const glm::quat& rot) {
                this->m_data.add_rotation(timepoint, rot.w, rot.x, rot.y, rot.z);
            }

            void addScale(const float timepoint, const float scale) {
                this->m_data.add_scale(timepoint, scale);
            }

            const std::string& name(void) const {
                return this->m_data.name_;
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

        void sample2(const float elapsed, const float animTick, const SkeletonInterface& interf,
            JointTransformArray& transformArr, const jointModifierRegistry_t& modifiers) const;
        float calcAnimTick(const float seconds) const;

    };


    class AnimationState {

    private:
        Timer m_localTimer;
        JointTransformArray m_finalTransform;
        jointModifierRegistry_t m_modifiers;
        unsigned int m_selectedAnimIndex = 0;
        float m_timeScale = 1.0f;
        float m_localTimeAccumulator = 0.0f;

    public:
        float getElapsed(void);
        JointTransformArray& getTransformArray(void);
        unsigned int getSelectedAnimeIndex(void) const;

        void setSelectedAnimeIndex(const unsigned int index);
        void setTimeScale(const float scale);

        void addModifier(const jointID_t jid, std::shared_ptr<IJointModifier> mod);
        const jointModifierRegistry_t& getModifiers(void) const {
            return this->m_modifiers;
        }

    };


    void updateAnimeState(AnimationState& state, const std::vector<Animation>& anims, const SkeletonInterface& skeletonInterf);

}
