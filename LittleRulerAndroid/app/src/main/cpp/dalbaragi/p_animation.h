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
        std::map<std::string, int32_t> m_map;
        std::vector<glm::mat4> m_boneOffsets;
        std::vector<glm::mat4> m_finalTransform;
        int32_t m_lastMadeIndex = -1;

    public:
        jointID_t getIndexOf(const std::string& jointName) const;
        jointID_t getOrMakeIndexOf(const std::string& jointName);

        void setOffsetMat(const jointID_t index, const glm::mat4& mat);
        const glm::mat4& getOffsetMat(const jointID_t index) const;

        void setFinalTransform(const jointID_t index, const glm::mat4& mat);
        const glm::mat4& getFinalTransform(const jointID_t index) const;

        jointID_t getSize(void) const;
        bool isEmpty(void) const;

        void sendUniform(const UniInterfAnime& uniloc) const;

    private:
        jointID_t upsizeAndGetIndex(void);
        bool isIndexValid(const jointID_t index) const;

    };


    struct JointKeyframeInfo {
        std::string m_name;
        std::map<float, glm::vec3> m_poses;
        std::map<float, glm::quat> m_rotates;
        std::map<float, float> m_scales;
    };


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

        void sample(const float animTick, const glm::mat4& parentTrans, SkeletonInterface& interf, const glm::mat4& globalInvMat) const;

    private:
        bool hasKeyframes(void) const;

        glm::vec3 makePosInterp(const float animTick) const;
        glm::quat makeRotateInterp(const float animTick) const;
        float makeScaleInterp(const float animTick) const;
        glm::mat4 makeTransformInterp(const float animTick) const;

    };


    class Animation {

    private:
        std::string m_name;
        JointNode m_rootNode;
        float m_tickPerSec, m_durationInTick;

    public:
        Animation(const std::string& name, const float tickPerSec, const float durationTick, JointNode&& rootNode);

        const std::string& getName(void) const { return this->m_name; }
        float getTickPerSec(void) const { return this->m_tickPerSec; }
        float getDurationInTick(void) const { return this->m_durationInTick; }

        void sample(const float animTick, SkeletonInterface& interf, const glm::mat4& globalInvMat) const;

    };


    class AnimationState {

    private:
        Timer m_localTimeline;
        std::string m_animationName;
        std::vector<glm::mat4> m_finalTransform;

    };

}