#include "p_light.h"

#include "s_logger_god.h"

#include <glm/gtc/matrix_transform.hpp>


namespace {

    constexpr unsigned int DEPTHMAP_RES = 1024 * 1;

}


// DepthmapForLights
namespace dal {

    DepthmapForLights::DepthmapForLights(void)
        : m_width(DEPTHMAP_RES)
        , m_height(DEPTHMAP_RES)
    {
        this->m_depthTex.init_depthMap(this->m_width, this->m_height);

        glGenFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        {
            const GLenum none = GL_NONE;
            glDrawBuffers(1, &none);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->m_depthTex.get(), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, this->m_depthTex.get());
            {
                if ( GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER) ) dalAbort("Framebuffer is not complete.");
            } glBindTexture(GL_TEXTURE_2D, 0);

            glClear(GL_DEPTH_BUFFER_BIT);
        } glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    DepthmapForLights::~DepthmapForLights(void) {
        glDeleteFramebuffers(1, &this->m_fbo);
    }

    DepthmapForLights::DepthmapForLights(DepthmapForLights&& other) noexcept {
        this->m_fbo = other.m_fbo;
        other.m_fbo = 0;

        this->m_width = other.m_width;
        this->m_height = other.m_height;

        this->m_depthTex = std::move(other.m_depthTex);
    }

    DepthmapForLights& DepthmapForLights::operator=(DepthmapForLights&& other) noexcept {
        this->m_fbo = other.m_fbo;
        other.m_fbo = 0;

        this->m_width = other.m_width;
        this->m_height = other.m_height;

        this->m_depthTex = std::move(other.m_depthTex);

        return *this;
    }


    void DepthmapForLights::sendUniform(const UniInterfLightedMesh& uniloc, int index) const {
        this->m_depthTex.sendUniform(uniloc.getDlightDepthMap(index));
    }

    void DepthmapForLights::clearBuffer(void) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void DepthmapForLights::startRender(void) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        glViewport(0, 0, this->m_width, this->m_height);
    }

    void DepthmapForLights::finishRender(void) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

}


namespace dal {

    DirectionalLight::DirectionalLight(void)
        : m_halfProjBoxEdgeLen(10.0f)
    {
        this->m_direction = glm::normalize(this->m_direction);
    }

    void DirectionalLight::setDirectin(const glm::vec3& v) {
        this->m_direction = glm::normalize(v);
    }

    void DirectionalLight::sendUniform(const UniInterfLightedMesh& uniloc, int index) const {
        uniloc.dlightColor(index, this->m_color);
        uniloc.dlightDirec(index, this->m_direction);

        auto projViewMat = this->makeProjMat() * this->makeViewMat();
        uniloc.dlightProjViewMat(index, projViewMat);

        this->m_shadowMap.sendUniform(uniloc, index);
    }

    void DirectionalLight::clearDepthBuffer(void) {
        this->m_shadowMap.clearBuffer();
    }

    void DirectionalLight::startRenderShadowmap(const UniInterfGeometry& uniloc) {
        uniloc.projectMat(this->makeProjMat());
        uniloc.viewMat(this->makeViewMat());
        this->m_shadowMap.startRender();
    }

    void DirectionalLight::finishRenderShadowmap(void) {
        this->m_shadowMap.finishRender();
    }

    glm::mat4 DirectionalLight::makeProjMat(void) const {
        return glm::ortho(
            -m_halfProjBoxEdgeLen, m_halfProjBoxEdgeLen,
            -m_halfProjBoxEdgeLen, m_halfProjBoxEdgeLen,
            -m_halfProjBoxEdgeLen, m_halfProjBoxEdgeLen
        );
    }

    glm::mat4 DirectionalLight::makeViewMat(void) const {
        return glm::lookAt(-this->m_direction + this->m_pos, this->m_pos, { 0.f, 1.f, 0.f });
    }

}


namespace dal {

    void PointLight::sendUniform(const UniInterfLightedMesh& uniloc, int index) const {
        uniloc.plightColor(index, this->m_color);
        uniloc.plightPos(index, this->mPos);
        uniloc.plightMaxDist(index, this->mMaxDistance);
    }

}