#include "u_loadinfo.h"

#include "s_logger_god.h"


using namespace std::string_literals;


namespace dal {
    namespace loadedinfo {

        int32_t JointInfoNoParent::getIndexOf(const std::string& jointName) const {
            auto iter = this->m_map.find(jointName);
            if ( this->m_map.end() == iter ) {
                return -1;
            }
            else {
                return iter->second;
            }
        }

        int32_t JointInfoNoParent::getOrMakeIndexOf(const std::string& jointName) {
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

        void JointInfoNoParent::setOffsetMat(const uint32_t index, const glm::mat4& mat) {
            dalAssert(index < this->m_boneOffsets.size());

            this->m_boneOffsets[index] = mat;
        }

        const glm::mat4& JointInfoNoParent::getOffsetMat(const uint32_t index) const {
            dalAssert(index < this->m_boneOffsets.size());

            return this->m_boneOffsets[index];
        }

        void JointInfoNoParent::setFinalTransform(const uint32_t index, const glm::mat4& mat) {
            dalAssert(index < this->m_boneOffsets.size());

            this->m_finalTransform[index] = mat;
        }

        const glm::mat4& JointInfoNoParent::getFinalTransform(const uint32_t index) const {
            dalAssert(index < this->m_boneOffsets.size());

            return this->m_finalTransform[index];
        }

        bool JointInfoNoParent::isEmpty(void) const {
            return this->m_map.empty();
        }

        void JointInfoNoParent::sendUniform(const UnilocAnimate& uniloc) const {
            for ( unsigned i = 0; i < this->m_finalTransform.size(); i++ ) {
                glUniformMatrix4fv(uniloc.u_poses[i], 1, GL_FALSE, &this->m_finalTransform[i][0][0]);
            }
        }

        int32_t JointInfoNoParent::upsizeAndGetIndex(void) {
            this->m_lastMadeIndex++;
            this->m_boneOffsets.emplace_back();
            this->m_finalTransform.emplace_back();
            return this->m_lastMadeIndex;
        }

    }  // namespace loadedinfo
}  // namespace dal


namespace dal {

    JointNode::JointNode(JointNode* const parent)
        : m_parent(parent)
    {

    }

    JointNode& JointNode::newChild(void) {
        this->m_children.emplace_back(this);
        return this->m_children.back();
    }

    void JointNode::setName(const std::string& name) {
        this->m_name = name;
    }

    void JointNode::setKeyframe(const loadedinfo::Animation::JointKeyframes& keyframe) {
        this->m_keyframe = keyframe;
    }

    void JointNode::setBindMat(const glm::mat4& mat) {
        this->m_bindMat = mat;
        this->m_invBindMat = glm::inverse(mat);
    }

    unsigned int JointNode::getNumNodesRecur(void) const {
        unsigned int count = 1;

        for ( auto& child : this->m_children ) {
            count += child.getNumNodesRecur();
        }

        return count;
    }

    void JointNode::sendBindPos(const UnilocAnimate& uniloc, const loadedinfo::JointInfoNoParent& jointInterface, glm::mat4 parentMat) const {
        const auto index = jointInterface.getIndexOf(this->m_name);
        const auto mat = this->m_bindMat * parentMat;
        if ( -1 != index ) {
            glUniformMatrix4fv(uniloc.u_poses[0], 1, GL_FALSE, &mat[0][0]);
        }
        
        for ( auto& child : this->m_children ) {
            child.sendBindPos(uniloc, jointInterface, mat);
        }
    }

}  // namespace dal


namespace dal {

    Animation::Animation(const std::string& name, JointNode&& rootNode)
        : m_name(name),
        m_rootNode(std::move(rootNode))
    {

    }

    unsigned int Animation::getNumNodes(void) const {
        return this->m_rootNode.getNumNodesRecur();
    }

    void Animation::sendBindPos(const UnilocAnimate& uniloc, const loadedinfo::JointInfoNoParent& jointInterface) const {
        this->m_rootNode.sendBindPos(uniloc, jointInterface, glm::mat4{ 1.0f });
    }

}  // namespace dal