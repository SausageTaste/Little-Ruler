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
            const auto parentID = interf.at(currentID).m_parentIndex;
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

    SkeletonInterface::BoneInfo& SkeletonInterface::at(const jointID_t index) {
        dalAssert(this->isIndexValid(index));
        return this->m_boneInfo[index];
    }

    const SkeletonInterface::BoneInfo& SkeletonInterface::at(const jointID_t index) const {
        const auto valid = this->isIndexValid(index);
        dalAssert(valid);
        return this->m_boneInfo[index];
    }

    const std::string& SkeletonInterface::getName(const jointID_t index) const {
        dalAssert(this->isIndexValid(index));
        for ( const auto& [name, id] : this->m_map ) {
            if ( id == index ) {
                return name;
            }
        }
        dalAbort("WTF");
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

    void JointTransformArray::sendUniform(const UniInterfAnime& uniloc) const {
        for ( unsigned i = 0; i < this->m_array.size(); i++ ) {
            uniloc.jointTransforms(i, this->m_array[i]);
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
        return (this->m_poses.size() + this->m_rotates.size() + this->m_scales.size()) != 0;
    }

    glm::vec3 Animation::JointNode::makePosInterp(const float animTick) const {
        return this->m_poses.empty() ? glm::vec3{} : makeInterpValue(animTick, this->m_poses);
    }

    glm::quat Animation::JointNode::makeRotateInterp(const float animTick) const {
        return this->m_rotates.empty() ? glm::quat{} : makeInterpValue(animTick, this->m_rotates);
    }

    float Animation::JointNode::makeScaleInterp(const float animTick) const {
        return this->m_scales.empty() ? 1.f : makeInterpValue(animTick, this->m_scales);
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

    void Animation::sample2(const float animTick, const SkeletonInterface& interf, const glm::mat4& globalInvMat, JointTransformArray& transformArr) const {
        static const auto k_spaceAnim2Model = glm::rotate(glm::mat4{ 1.f }, glm::radians(-90.f), glm::vec3{ 1.f, 0.f, 0.f });
        static const auto k_spaceModel2Anim = glm::inverse(k_spaceAnim2Model);

        const auto numBones = interf.getSize();
        transformArr.setSize(numBones);

        std::vector<glm::mat4> boneTransforms(numBones);
        dalAssert(numBones == this->m_joints.size());
        for ( int i = 0; i < numBones; ++i ) {
            boneTransforms[i] = this->m_joints[i].makeTransform(animTick);
        }

        for ( dal::jointID_t i = 0; i < numBones; ++i ) {
            dal::jointID_t curBone = i;
            glm::mat4 totalTrans = interf.at(i).m_boneOffset;
            while ( curBone != -1 ) {
                totalTrans = interf.at(curBone).m_spaceToParent * boneTransforms[curBone] * totalTrans;
                curBone = interf.at(curBone).m_parentIndex;
            }
            transformArr.setTransform(i, k_spaceAnim2Model * totalTrans * k_spaceModel2Anim);
        }

        static bool once = false;
        if ( !once ) {
            for ( dal::jointID_t i = 0; i < numBones; ++i ) {

            }
            once = true;
        }

        return;
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

    void updateAnimeState(AnimationState& state, const std::vector<Animation>& anims, const SkeletonInterface& skeletonInterf, const glm::mat4& globalMatInv) {
        const auto selectedAnimIndex = state.getSelectedAnimeIndex();
        if ( selectedAnimIndex >= anims.size() ) {
            //dalError("Selected animation index out of range");
            return;
        }

        const auto& anim = anims[selectedAnimIndex];
        const auto elapsed = state.getElapsed();
        const auto animTick = anim.calcAnimTick(elapsed);
        //anim.sample(animTick, skeletonInterf, globalMatInv, state.getTransformArray());
        anim.sample2(animTick, skeletonInterf, globalMatInv, state.getTransformArray());
    }

}
