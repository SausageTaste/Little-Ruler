#include "p_animation.h"

#include "s_logger_god.h"


namespace dal {

    int32_t SkeletonInterface::getIndexOf(const std::string& jointName) const {
        auto iter = this->m_map.find(jointName);
        if ( this->m_map.end() == iter ) {
            return -1;
        }
        else {
            return iter->second;
        }
    }

    int32_t SkeletonInterface::getOrMakeIndexOf(const std::string& jointName) {
        const auto index = this->getIndexOf(jointName);

        if ( -1 == index ) {
            const auto newIndex = this->upsizeAndGetIndex();
            this->m_map.emplace(jointName, newIndex);
            return newIndex;
        }
        else {
            return index;
        }
    }

    void SkeletonInterface::setOffsetMat(const int32_t index, const glm::mat4& mat) {
        dalAssert(this->isIndexValid(index));

        this->m_boneOffsets[index] = mat;
    }

    const glm::mat4& SkeletonInterface::getOffsetMat(const int32_t index) const {
        dalAssert(this->isIndexValid(index));

        return this->m_boneOffsets[index];
    }

    void SkeletonInterface::setFinalTransform(const int32_t index, const glm::mat4& mat) {
        dalAssert(this->isIndexValid(index));

        this->m_finalTransform[index] = mat;
    }

    const glm::mat4& SkeletonInterface::getFinalTransform(const int32_t index) const {
        dalAssert(this->isIndexValid(index));

        return this->m_finalTransform[index];
    }

    bool SkeletonInterface::isEmpty(void) const {
        return this->m_map.empty();
    }

    void SkeletonInterface::sendUniform(const UnilocAnimate& uniloc) const {
        for ( unsigned i = 0; i < this->m_finalTransform.size(); i++ ) {
            glUniformMatrix4fv(uniloc.u_poses[i], 1, GL_FALSE, &this->m_finalTransform[i][0][0]);
        }
    }

    int32_t SkeletonInterface::upsizeAndGetIndex(void) {
        this->m_lastMadeIndex++;
        this->m_boneOffsets.emplace_back();
        this->m_finalTransform.emplace_back();
        return this->m_lastMadeIndex;
    }

    bool SkeletonInterface::isIndexValid(const int32_t index) const {
        if ( index < 0 ) return false;
        else if ( static_cast<unsigned int>(index) >= this->m_boneOffsets.size() ) return false;
        else return true;
    }

}  // namespace dal


namespace dal {

    JointNode::JointNode(const JointKeyframeInfo& info, const glm::mat4& transform, JointNode* const parent)
        : m_name(info.m_name),
        m_transform(transform),
        m_parent(parent)
    {
        this->m_poses.reserve(info.m_poses.size());
        for ( const auto& x : info.m_poses ) {
            this->m_poses.emplace_back(x);
        }

        this->m_rotates.reserve(info.m_rotates.size());
        for ( const auto& x : info.m_rotates ) {
            this->m_rotates.emplace_back(x);
        }

        this->m_scales.reserve(info.m_scales.size());
        for ( const auto& x : info.m_scales ) {
            this->m_scales.emplace_back(x);
        }
    }

    JointNode::JointNode(const std::string& name, const glm::mat4& transform, JointNode* const parent)
        : m_name(name),
        m_transform(transform),
        m_parent(parent)
    {
        this->m_poses.reserve(0);
        this->m_rotates.reserve(0);
        this->m_scales.reserve(0);
    }

    JointNode* JointNode::emplaceChild(const JointKeyframeInfo& info, const glm::mat4& transform, JointNode* const parent) {
        this->m_children.emplace_back(info, transform, parent);
        return &(this->m_children.back());
    }

    JointNode* JointNode::emplaceChild(const std::string& name, const glm::mat4& transform, JointNode* const parent) {
        this->m_children.emplace_back(name, transform, parent);
        return &(this->m_children.back());
    }

    void JointNode::sample(const float animTick, const glm::mat4& parentTrans, std::vector<glm::mat4>& result, const SkeletonInterface& interf) {

    }

}  // namespace dal


namespace dal {

    Animation::Animation(const std::string& name, const float tickPerSec, const float durationTick, JointNode&& rootNode)
        : m_name(name),
        m_rootNode(std::move(rootNode)),
        m_tickPerSec(tickPerSec),
        m_durationInTick(durationTick)
    {

    }

    std::vector<glm::mat4> Animation::sample(const float scale, const SkeletonInterface& interf) {
        const float sampleTick = this->m_durationInTick * std::fmod(scale, 1.0f);

        std::vector<glm::mat4> result;
        this->m_rootNode.sample(sampleTick, result, interf);
        return result;
    }

}  // namespace dal