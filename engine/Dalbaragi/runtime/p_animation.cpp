#include "p_animation.h"

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>

#include <d_logger.h>
#include <u_math.h>


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
    // WTF such a broken Engilsh that even I can't understand!!
    template <typename T>
    size_t findIndexToStartInterp(const std::vector<std::pair<float, T>>& container, const float criteria) {
        dalAssert(0 != container.size());

        for ( size_t i = 0; i < container.size() - 1; i++ ) {
            if ( criteria < container[i + 1].first ) {
                return i;
            }
        }

        return container.size() - 1;
    }

    template <typename T>
    T makeInterpValue(const float animTick, const std::vector<std::pair<float, T>>& container) {
        dalAssert(0 != container.size());

        if ( 1 == container.size() ) {
            return container[0].second;
        }

        const auto startIndex = findIndexToStartInterp(container, animTick);
        const auto nextIndex = startIndex + 1;
        if ( nextIndex >= container.size() ) {
            return container.back().second;
        }

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

    std::vector<dal::jointID_t> makeParentList(const dal::jointID_t id, const dal::SkeletonInterface& interf) {
        std::vector<dal::jointID_t> result;

        dal::jointID_t currentID = id;
        while ( true ) {
            const auto parentID = interf.at(currentID).parentIndex();
            if ( parentID < 0 ) {
                return result;
            }
            else {
                result.push_back(parentID);
                currentID = parentID;
            }
        }
    }

}


// JointInfo
namespace dal {

    glm::vec3 JointInfo::localPos(void) const {
        const auto result = decomposeTransform(this->offset());
        return result.first;
    }

    void JointInfo::setOffset(const glm::mat4& mat) {
        this->m_jointOffset = mat;
        this->m_jointOffsetInv = glm::inverse(this->m_jointOffset);
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
            this->m_boneInfo[newIndex].setName(jointName);
            return newIndex;
        }
        else {
            return index;
        }
    }

    JointInfo& SkeletonInterface::at(const jointID_t index) {
        dalAssert(this->isIndexValid(index));
        return this->m_boneInfo[index];
    }

    const JointInfo& SkeletonInterface::at(const jointID_t index) const {
        dalAssert(this->isIndexValid(index));
        return this->m_boneInfo[index];
    }

    jointID_t SkeletonInterface::getSize(void) const {
        return static_cast<int32_t>(this->m_boneInfo.size());
    }

    bool SkeletonInterface::isEmpty(void) const {
        return this->m_map.empty();
    }

    void SkeletonInterface::clear(void) {
        this->m_map.clear();
        this->m_boneInfo.clear();
        this->m_lastMadeIndex = -1;
    }

    // Private

    jointID_t SkeletonInterface::upsizeAndGetIndex(void) {
        this->m_lastMadeIndex++;
        this->m_boneInfo.emplace_back();
        return this->m_lastMadeIndex;
    }

    bool SkeletonInterface::isIndexValid(const jointID_t index) const {
        if ( index < 0 ) {
            return false;
        }
        else if ( static_cast<unsigned int>(index) >= this->m_boneInfo.size() ) {
            return false;
        }
        else {
            return true;
        }
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

    void JointTransformArray::sendUniform(const UniInterf_Skeleton& uniloc) const {
        for ( unsigned i = 0; i < this->m_array.size(); i++ ) {
            uniloc.jointTrans(i, this->m_array[i]);
        }
    }

}


namespace dal {

    glm::mat4 Animation::JointNode::makeTransform(const float animTick) const {
        const auto pos = this->makePosInterp(animTick);
        const auto rotate = this->makeRotateInterp(animTick);
        const auto scale = this->makeScaleInterp(animTick);

        const glm::mat4 identity{ 1.0f };
        const auto posMat = glm::translate(identity, pos);
        const auto rotateMat = glm::mat4_cast(rotate);
        const auto scaleMat = glm::scale(identity, glm::vec3{ scale });

        return posMat * rotateMat * scaleMat;
    }

    // Private

    bool Animation::JointNode::hasKeyframes(void) const {
        return (this->m_data.m_positions.size() + this->m_data.m_rotations.size() + this->m_data.m_scales.size()) != 0;
    }

    glm::vec3 Animation::JointNode::makePosInterp(const float animTick) const {
        return this->m_data.m_positions.empty() ? glm::vec3{} : makeInterpValue(animTick, this->m_data.m_positions);
    }

    glm::quat Animation::JointNode::makeRotateInterp(const float animTick) const {
        return this->m_data.m_rotations.empty() ? glm::quat{} : makeInterpValue(animTick, this->m_data.m_rotations);
    }

    float Animation::JointNode::makeScaleInterp(const float animTick) const {
        return this->m_data.m_scales.empty() ? 1.f : makeInterpValue(animTick, this->m_data.m_scales);
    }

}  // namespace dal


namespace dal {

    Animation::Animation(const std::string& name, const float tickPerSec, const float durationTick)
        : m_name(name)
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

    void Animation::sample2(
        const float elapsed,
        const float animTick,
        const SkeletonInterface& interf,
        JointTransformArray& transformArr,
        const jointModifierRegistry_t& modifiers
    ) const {
        const auto numBones = interf.getSize();
        transformArr.setSize(numBones);

        std::vector<glm::mat4> boneTransforms(numBones);
        dalAssert(numBones == this->m_joints.size());
        for ( int i = 0; i < numBones; ++i ) {
            const auto found = modifiers.find(i);
            if ( modifiers.end() != found ) {
                boneTransforms[i] = found->second->makeTransform(elapsed, i, interf);
            }
            else {
                boneTransforms[i] = this->m_joints[i].makeTransform(animTick);
            }
        }

        for ( dal::jointID_t i = 0; i < numBones; ++i ) {
            dal::jointID_t curBone = i;
            glm::mat4 totalTrans = interf.at(i).offsetInv();
            while ( curBone != -1 ) {
                totalTrans = interf.at(curBone).toParent() * boneTransforms[curBone] * totalTrans;
                curBone = interf.at(curBone).parentIndex();
            }
            transformArr.setTransform(i, totalTrans);
        }
    }

    float Animation::calcAnimTick(const float seconds) const {
        const auto animDuration = this->getDurationInTick();
        const auto animTickPerSec = this->getTickPerSec();
        float TimeInTicks = seconds * animTickPerSec;
        return fmod(TimeInTicks, animDuration);
    }

}  // namespace dal


namespace dal {

    float AnimationState::getElapsed(void) {
        const auto deltaTime = this->m_localTimer.checkGetElapsed();
        this->m_localTimeAccumulator += deltaTime * this->m_timeScale;
        return this->m_localTimeAccumulator;
    }

    JointTransformArray& AnimationState::getTransformArray(void) {
        return this->m_finalTransform;
    }

    unsigned int AnimationState::getSelectedAnimeIndex(void) const {
        return this->m_selectedAnimIndex;
    }

    void AnimationState::setSelectedAnimeIndex(const unsigned int index) {
        if ( this->m_selectedAnimIndex != index ) {
            this->m_selectedAnimIndex = index;
            this->m_localTimeAccumulator = 0.0f;
        }
    }

    void AnimationState::setTimeScale(const float scale) {
        this->m_timeScale = scale;
    }

    void AnimationState::addModifier(const jointID_t jid, std::shared_ptr<IJointModifier> mod) {
        this->m_modifiers.emplace(jid, std::move(mod));
    }

}


// Functions
namespace dal {

    void updateAnimeState(AnimationState& state, const std::vector<Animation>& anims, const SkeletonInterface& skeletonInterf) {
        const auto selectedAnimIndex = state.getSelectedAnimeIndex();
        if ( selectedAnimIndex >= anims.size() ) {
            //dalError(fmt::format("Selected animation's index is out of range: {}", selectedAnimIndex));
            return;
        }

        const auto& anim = anims[selectedAnimIndex];
        const auto elapsed = state.getElapsed();
        const auto animTick = anim.calcAnimTick(elapsed);
        anim.sample2(elapsed, animTick, skeletonInterf, state.getTransformArray(), state.getModifiers());
    }

}
