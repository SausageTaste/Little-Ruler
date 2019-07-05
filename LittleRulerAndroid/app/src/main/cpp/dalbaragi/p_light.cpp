#include "p_light.h"

#include "s_logger_god.h"

#include <glm/gtc/matrix_transform.hpp>


namespace {

    constexpr unsigned int DEPTHMAP_RES = 1024 * 2;

}


namespace dal {

    DepthmapForLights::DepthmapForLights(void)
        : width(DEPTHMAP_RES),
        height(DEPTHMAP_RES)
    {
        this->mDepthmap = ResourceMaster::getUniqueTexture();
        this->mDepthmap->init_depthMap(this->width, this->height);

        glGenFramebuffers(1, &mFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
        {
            const GLenum none = GL_NONE;
            glDrawBuffers(1, &none);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->mDepthmap->get(), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, this->mDepthmap->get());
            {
                if ( GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER) ) dalAbort("Framebuffer is not complete.");
            } glBindTexture(GL_TEXTURE_2D, 0);

            glClear(GL_DEPTH_BUFFER_BIT);
        } glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    DepthmapForLights::~DepthmapForLights(void) {
        ResourceMaster::dumpUniqueTexture(this->mDepthmap);
        glDeleteFramebuffers(1, &this->mFBO);
    }

    DepthmapForLights::DepthmapForLights(DepthmapForLights&& other) noexcept {
        this->mFBO = other.mFBO;
        other.mFBO = 0;

        this->width = other.width;
        this->height = other.height;

        this->mDepthmap = other.mDepthmap;
    }

    DepthmapForLights& DepthmapForLights::operator=(DepthmapForLights&& other) noexcept {
        this->mFBO = other.mFBO;
        other.mFBO = 0;

        this->width = other.width;
        this->height = other.height;

        this->mDepthmap = other.mDepthmap;

        return *this;
    }

    GLuint DepthmapForLights::getTextureID(void) {
        return mDepthmap->get();
    }

    const Texture* DepthmapForLights::getDepthMap(void) const {
        return mDepthmap;
    }

    void DepthmapForLights::clearBuffer(void) {
        glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void DepthmapForLights::startRender(void) {
        glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
        glViewport(0, 0, width, height);
    }

    void DepthmapForLights::finishRender(void) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

}


namespace dal {

    DirectionalLight::DirectionalLight(void) {
        this->m_direction = glm::normalize(this->m_direction);
    }

    void DirectionalLight::setDirectin(const glm::vec3& direction) {
        this->m_direction = direction;
    }

    void DirectionalLight::setDirectin(const float x, const float y, const float z) {
        this->m_direction.x = x;
        this->m_direction.y = y;
        this->m_direction.z = z;
    }

    const glm::vec3& DirectionalLight::getDirection(void) const {
        return this->m_direction;
    }

    void DirectionalLight::sendUniform(const UniInterfLightedMesh& uniloc, int index) const {
        uniloc.dlightColor(index, this->m_color);
        uniloc.dlightDirec(index, this->m_direction);

        auto projViewMat = this->makeProjMat() * this->makeViewMat();
        uniloc.dlightProjViewMat(index, projViewMat);

        this->mShadowMap.getDepthMap()->sendUniform(uniloc.getDlightDepthMap(index), -1, 1 + index);
    }

    void DirectionalLight::clearDepthBuffer(void) {
        this->mShadowMap.clearBuffer();
    }

    void DirectionalLight::startRenderShadowmap(const UniInterfGeometry& uniloc) {
        uniloc.projectMat(this->makeProjMat());
        uniloc.viewMat(this->makeViewMat());
        mShadowMap.startRender();
    }

    void DirectionalLight::finishRenderShadowmap(void) {
        mShadowMap.finishRender();
    }

    glm::mat4 DirectionalLight::makeProjMat(void) const {
        return glm::ortho(
            -mHalfShadowEdgeSize, mHalfShadowEdgeSize,
            -mHalfShadowEdgeSize, mHalfShadowEdgeSize,
            -mHalfShadowEdgeSize, mHalfShadowEdgeSize
        );
    }

    glm::mat4 DirectionalLight::makeViewMat(void) const {
        return glm::lookAt(-this->m_direction, { 0, 0, 0 }, { 0,1,0 });
    }

    GLuint DirectionalLight::getShadowMapTexture(void) {
        return mShadowMap.getTextureID();
    }

    const Texture* DirectionalLight::getShadowMap(void) const {
        return mShadowMap.getDepthMap();
    }

}


namespace dal {

    void PointLight::sendUniform(const UniInterfLightedMesh& uniloc, int index) const {
        uniloc.plightColor(index, this->m_color);
        uniloc.plightPos(index, this->mPos);
        uniloc.plightMaxDist(index, this->mMaxDistance);
    }

}