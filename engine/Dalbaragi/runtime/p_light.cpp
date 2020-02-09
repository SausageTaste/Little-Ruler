#include "p_light.h"

#include <glm/gtc/matrix_transform.hpp>

#include <d_logger.h>


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


    void DepthmapForLights::sendUniform(const SamplerInterf& uniloc) const {
        this->m_depthTex.sendUniform(uniloc);
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


// DirectionalLight
namespace dal {

    DirectionalLight::DirectionalLight(void)
        : m_halfProjBoxEdgeLen(30.0f)
    {
        this->m_direction = glm::normalize(this->m_direction);
    }

    void DirectionalLight::setDirectin(const glm::vec3& v) {
        this->m_direction = glm::normalize(v);
    }

    void DirectionalLight::sendUniform(const UniInterfLightedMesh::DirecLight& uniloc) const {
        uniloc.color(this->m_color);
        uniloc.direc(this->m_direction);
        uniloc.projViewMat(this->makeProjMat() * this->makeViewMat());
        this->m_shadowMap.sendUniform(uniloc.getDepthMap());
    }

    void DirectionalLight::sendUniform(const unsigned index, const UniInterf_Lighting& uniloc) const {
        uniloc.dlight_color(index, this->m_color);
        uniloc.dlight_direc(index, this->m_direction);
        uniloc.dlight_projViewMat(index, this->makeProjMat() * this->makeViewMat());
        this->m_shadowMap.sendUniform(uniloc.dlight_shadowmap(index));
    }

    void DirectionalLight::clearDepthBuffer(void) {
        this->m_shadowMap.clearBuffer();
    }

    void DirectionalLight::startRenderShadowmap(const UniInterfGeometry& uniloc) {
        uniloc.projectMat(this->makeProjMat());
        uniloc.viewMat(this->makeViewMat());
        this->m_shadowMap.startRender();
    }

    void DirectionalLight::startRenderShadowmap(const UniRender_StaticDepth& uniloc) {
        uniloc.projMat(this->makeProjMat());
        uniloc.viewMat(this->makeViewMat());
        this->m_shadowMap.startRender();
    }

    void DirectionalLight::startRenderShadowmap(const UniRender_AnimatedDepth& uniloc) {
        uniloc.projMat(this->makeProjMat());
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


// PointLight
namespace dal {

    void PointLight::sendUniform(const UniInterfLightedMesh::PointLight& uniloc) const {
        uniloc.color(this->m_color);
        uniloc.pos(this->mPos);
    }

    void PointLight::sendUniform(unsigned index, const UniInterf_Lighting& uniloc) const {
        uniloc.plight_color(index, this->m_color);
        uniloc.plight_pos(index, this->mPos);
    }

}


// SpotLight
namespace dal {

    SpotLight::SpotLight(void)
        : m_direc(0.f, -1.f, 0.f)
        , m_color(1.f, 1.f, 1.f)
        , m_endFadeRadians(glm::radians(45.f))
        , m_startFade(cos(glm::radians(40.f)))
        , m_endFade(cos(m_endFadeRadians))
    {

    }

    void SpotLight::sendUniform(const UniInterfLightedMesh::SpotLight& uniloc) const {
        uniloc.pos(this->m_pos);
        uniloc.direc(this->m_direc);
        uniloc.color(this->m_color);
        uniloc.startFade(this->m_startFade);
        uniloc.endFade(this->m_endFade);

        auto projViewMat = this->makeProjMat() * this->makeViewMat();
        uniloc.projViewMat(projViewMat);

        this->m_shadowMap.sendUniform(uniloc.getDepthMap());
    }

    void SpotLight::sendUniform(const UniInterf_Lighting& uniloc, const unsigned index) const {
        uniloc.slight_poses(index, this->m_pos);
        uniloc.slight_direcs(index, this->m_direc);
        uniloc.slight_colors(index, this->m_color);
        uniloc.slight_fadeStart(index, this->m_startFade);
        uniloc.slight_fadeEnd(index, this->m_endFade);

        const auto projViewMat = this->makeProjMat() * this->makeViewMat();
        uniloc.slight_projViewMat(index, projViewMat);

        this->m_shadowMap.sendUniform(uniloc.slight_shadowmap(index));
    }

    // Shadow mapping

    glm::mat4 SpotLight::makeProjMat(void) const {
        return glm::perspective(this->m_endFadeRadians * 2.f, 1.f, 1.f, 100.f);
    }

    glm::mat4 SpotLight::makeViewMat(void) const {
        const auto eye = this->m_pos;
        const auto center = this->m_pos + this->m_direc;
        return glm::lookAt(eye, center, glm::vec3{ 0.f, 1.f, 0.f });
    }

}
