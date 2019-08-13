#include "p_animation.h"

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>

#include "s_logger_god.h"


using namespace fmt::literals;


namespace {

    float interpolate(const float start, const float end, const float factor) {
        const auto delta = end - start;
        return start + factor * delta;
    }

    glm::vec3 interpolate(const glm::vec3& start, const glm::vec3& end, const float factor) {
        const auto delta = end - start;
        return start + factor * delta;
    }

    glm::quat interpolate(const glm::quat& start, const glm::quat& end, const float factor) {
        return glm::slerp(start, end, factor);
    }
    
    // Pass it container with size 0 and iterator to index of unsigned{ -1 }.
    template <typename T>
    size_t findIndexToStartInterp(const std::vector<std::pair<float, T>>& container, const float criteria) {
        dalAssert(0 != container.size());

        for ( size_t i = 0; i < container.size() - 1; i++ ) {
            if ( criteria < container[i + 1].first ) {
                return i;
            }
        }

        dalAssert(false);
        return 0;
    }

    template <typename T>
    T makeInterpValue(const float animTick, const std::vector<std::pair<float, T>>& container) {
        dalAssert(0 != container.size());
        if ( 1 == container.size() ) {
            return container[0].second;
        }

        const auto startIndex = findIndexToStartInterp(container, animTick);
        const auto nextIndex = startIndex + 1;
        dalAssert(nextIndex < container.size());

        const auto deltaTime = container[nextIndex].first - container[startIndex].first;
        auto factor = (animTick - container[startIndex].first) / deltaTime;
        if ( 0.0f <= factor && factor <= 1.0f ) {
            const auto start = container[startIndex].second;
            const auto end = container[nextIndex].second;
            return interpolate(start, end, factor);
        }
        else {
            const auto start = container[startIndex].second;
            const auto end = container[nextIndex].second;
            return interpolate(start, end, 0.0f);
        }
    }

}


namespace dal {

    jointID_t SkeletonInterface::getIndexOf(const std::string& jointName) const {
        auto iter = this->m_map.find(jointName);
        if ( this->m_map.end() == iter ) {
            return -1;
        }
        else {
            return iter->second;
        }
    }

    jointID_t SkeletonInterface::getOrMakeIndexOf(const std::string& jointName) {
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

    void SkeletonInterface::setOffsetMat(const jointID_t index, const glm::mat4& mat) {
        dalAssert(this->isIndexValid(index));

        this->m_boneOffsets[index] = mat;
    }

    const glm::mat4& SkeletonInterface::getOffsetMat(const jointID_t index) const {
        dalAssert(this->isIndexValid(index));

        return this->m_boneOffsets[index];
    }

    jointID_t SkeletonInterface::getSize(void) const {
        return static_cast<int32_t>(this->m_boneOffsets.size());
    }

    bool SkeletonInterface::isEmpty(void) const {
        return this->m_map.empty();
    }

    // Private

    jointID_t SkeletonInterface::upsizeAndGetIndex(void) {
        this->m_lastMadeIndex++;
        this->m_boneOffsets.emplace_back();
        return this->m_lastMadeIndex;
    }

    bool SkeletonInterface::isIndexValid(const jointID_t index) const {
        if ( index < 0 ) return false;
        else if ( static_cast<unsigned int>(index) >= this->m_boneOffsets.size() ) return false;
        else return true;
    }

}  // namespace dal


namespace dal {

    void JointTransformArray::setSize(const jointID_t size) {
        this->m_array.resize(size);
    }

    void JointTransformArray::setTransform(const jointID_t index, const glm::mat4& mat) {
        dalAssert(index >= 0);
        dalAssert(static_cast<size_t>(index) < this->m_array.size());
        this->m_array[index] = mat;
    }

    void JointTransformArray::sendUniform(const UniInterfAnime& uniloc) const {
        for ( unsigned i = 0; i < this->m_array.size(); i++ ) {
            uniloc.jointTransforms(i, this->m_array[i]);
        }
    }

}


namespace dal {

    Animation::JointNode::JointNode(const JointKeyframeInfo& info, const glm::mat4& transform, JointNode* const parent)
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

    Animation::JointNode::JointNode(const std::string& name, const glm::mat4& transform, JointNode* const parent)
        : m_name(name),
        m_transform(transform),
        m_parent(parent)
    {
        this->m_poses.reserve(0);
        this->m_rotates.reserve(0);
        this->m_scales.reserve(0);
    }

    Animation::JointNode* Animation::JointNode::emplaceChild(const JointKeyframeInfo& info, const glm::mat4& transform, JointNode* const parent) {
        this->m_children.emplace_back(info, transform, parent);
        return &(this->m_children.back());
    }

    Animation::JointNode* Animation::JointNode::emplaceChild(const std::string& name, const glm::mat4& transform, JointNode* const parent) {
        this->m_children.emplace_back(name, transform, parent);
        return &(this->m_children.back());
    }

    void Animation::JointNode::sample(const float animTick, const glm::mat4& parentTrans, const SkeletonInterface& interf, const glm::mat4& globalInvMat,
        JointTransformArray& transformArr) const {
        const auto nodeTransform = parentTrans * (this->hasKeyframes() ? this->makeTransformInterp(animTick) : this->m_transform);

        const auto jointInferfIndex = interf.getIndexOf(this->m_name);
        if ( -1 != jointInferfIndex ) {
            const auto finalTrans = globalInvMat * nodeTransform * interf.getOffsetMat(jointInferfIndex);
            transformArr.setTransform(jointInferfIndex, finalTrans);
        }

        for ( auto& child : this->m_children ) {
            child.sample(animTick, nodeTransform, interf, globalInvMat, transformArr);
        }
    }

    // Private

    bool Animation::JointNode::hasKeyframes(void) const {
        return (this->m_poses.size() + this->m_rotates.size() + this->m_scales.size()) != 0;
    }

    glm::vec3 Animation::JointNode::makePosInterp(const float animTick) const {
        if ( 0 == this->m_poses.size() ) {
            dalAbort("Trying to interpolate poses when there is no pos keyframes for: {}"_format(this->m_name));
        }

        return makeInterpValue(animTick, this->m_poses);
    }

    glm::quat Animation::JointNode::makeRotateInterp(const float animTick) const {
        if ( 0 == this->m_rotates.size() ) {
            dalAbort("Trying to interpolate rotates when there is no rotate keyframes for: {}"_format(this->m_name));
        }

        return makeInterpValue(animTick, this->m_rotates);
    }

    float Animation::JointNode::makeScaleInterp(const float animTick) const {
        if ( 0 == this->m_scales.size() ) {
            dalAbort("Trying to interpolate scales when there is no scale keyframes for: {}"_format(this->m_name));
        }

        return makeInterpValue(animTick, this->m_scales);
    }

    glm::mat4 Animation::JointNode::makeTransformInterp(const float animTick) const {
        const auto pos = this->makePosInterp(animTick);
        const auto rotate = this->makeRotateInterp(animTick);
        const auto scale = this->makeScaleInterp(animTick);

        const glm::mat4 identity{ 1.0f };
        const auto posMat = glm::translate(identity, pos);
        const auto rotateMat = glm::mat4_cast(rotate);
        const auto scaleMat = glm::scale(identity, glm::vec3{ scale });

        return posMat * rotateMat * scaleMat;
    }

}  // namespace dal


namespace dal {

    Animation::Animation(const std::string& name, const float tickPerSec, const float durationTick, JointNode&& rootNode)
        : m_name(name)
        , m_rootNode(std::move(rootNode))
        , m_tickPerSec(tickPerSec)
        , m_durationInTick(durationTick)
    {
        if ( 0.0f == this->m_durationInTick ) {
            this->m_durationInTick = 1.0f;
        }
        if ( 0.0f == this->m_tickPerSec ) {
            this->m_tickPerSec = 25.0f;
        }
    }

    void Animation::sample(const float animTick, const SkeletonInterface& interf, const glm::mat4& globalInvMat,
        JointTransformArray& transformArr) const {
        //const float sampleTick = this->m_durationInTick * std::fmod(scale, 1.0f);
        transformArr.setSize(interf.getSize());
        this->m_rootNode.sample(animTick, glm::mat4{ 1.0f }, interf, globalInvMat, transformArr);
    }
    
    float Animation::calcAnimTick(const float seconds) const {
        const auto animDuration = this->getDurationInTick();
        const auto animTickPerSec = this->getTickPerSec();
        float TimeInTicks = seconds * animTickPerSec;
        return fmod(TimeInTicks, animDuration);
    }

}  // namespace dal


// Functions
namespace dal {

    void updateAnimeState(AnimationState& state, const std::vector<Animation>& anims,
        const SkeletonInterface& skeletonInterf, const glm::mat4& globalMatInv)
    {
        const auto selectedAnimIndex = state.getSelectedAnimeIndex();
        if ( selectedAnimIndex >= anims.size() ) {
            //dalError("Selected animation index out of range");
            return;
        }

        const auto& anim = anims[selectedAnimIndex];
        const auto elapsed = state.getElapsed();
        const auto animTick = anim.calcAnimTick(elapsed);
        anim.sample(animTick, skeletonInterf, globalMatInv, state.getTransformArray());
    }

}