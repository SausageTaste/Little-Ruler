#include "p_light.h"

#include "s_logger_god.h"

#include <glm/gtc/matrix_transform.hpp>


namespace {

	constexpr unsigned int DEPTHMAP_RES = 1024 * 5;

}


namespace dal {

	DepthmapForLights::DepthmapForLights(void)
	:	width(DEPTHMAP_RES),
		height(DEPTHMAP_RES)
	{
		this->mDepthmap = ResourceMaster::getUniqueTexture();
		this->mDepthmap->init_depthMap(this->width, this->height);

		glGenFramebuffers(1, &mFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, mFBO); {
			const GLenum none = GL_NONE;
			glDrawBuffers(1, &none);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->mDepthmap->get(), 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->mDepthmap->get()); {
				if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) dalAbort("Framebuffer is not complete.");
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

	void DepthmapForLights::startRender(void) {
		glBindFramebuffer(GL_FRAMEBUFFER, mFBO);

		glViewport(0, 0, width, height);
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	void DepthmapForLights::finishRender(void) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}

}


namespace dal {

	DirectionalLight::DirectionalLight(void) {
		this->mDirection = glm::normalize(this->mDirection);
	}

	void DirectionalLight::sendUniform(const UnilocGeneral& uniloc, int index) const {
		glUniform3f(uniloc.uDlightColors[index], this->m_color.r, this->m_color.g, this->m_color.b);
		glUniform3f(uniloc.uDlightDirecs[index], mDirection.x, mDirection.y, mDirection.z);

		auto projViewMat = this->makeProjViewMap();
		glUniformMatrix4fv(uniloc.uDlightProjViewMat[index], 1, GL_FALSE, &projViewMat[0][0]);

		this->mShadowMap.getDepthMap()->sendUniform(uniloc.uDlightDepthMap[index], 0, 1 + index);
	}

	void DirectionalLight::sendUniform(const UnilocWaterry& uniloc, int index) const {
		glUniform3f(uniloc.uDlightColors[index], this->m_color.r, this->m_color.g, this->m_color.b);
		glUniform3f(uniloc.uDlightDirecs[index], mDirection.x, mDirection.y, mDirection.z);

		auto projViewMat = this->makeProjViewMap();
		glUniformMatrix4fv(uniloc.uDlightProjViewMat[index], 1, GL_FALSE, &projViewMat[0][0]);

		this->mShadowMap.getDepthMap()->sendUniform(uniloc.uDlightDepthMap[index], 0, 1 + index);
	}

	void DirectionalLight::startRenderShadowmap(const UnilocDepthmp& uniloc) {
		auto mat = this->makeProjViewMap();
		glUniformMatrix4fv(uniloc.uProjViewMat, 1, GL_FALSE, &mat[0][0]);
		mShadowMap.startRender();
	}

	void DirectionalLight::finishRenderShadowmap(void) {
		mShadowMap.finishRender();
	}

	glm::mat4 DirectionalLight::makeProjViewMap(void) const {
		return glm::ortho(
			-mHalfShadowEdgeSize, mHalfShadowEdgeSize,
			-mHalfShadowEdgeSize, mHalfShadowEdgeSize,
			-mHalfShadowEdgeSize, mHalfShadowEdgeSize
		) * glm::lookAt(-mDirection, { 0, 0, 0 }, { 0,1,0 });
	}

	GLuint DirectionalLight::getShadowMapTexture(void) {
		return mShadowMap.getTextureID();
	}

	const Texture* DirectionalLight::getShadowMap(void) {
		return mShadowMap.getDepthMap();
	}

}


namespace dal {

	void PointLight::sendUniform(const UnilocGeneral& uniloc, int index) const {
		glUniform3f(uniloc.uPlightColors[index], this->m_color.r, this->m_color.g, this->m_color.b);
		glUniform3f(uniloc.uPlightPoses[index], mPos.x, mPos.y, mPos.z);
		glUniform1f(uniloc.uPlightMaxDists[index], mMaxDistance);
	}

	void PointLight::sendUniform(const UnilocWaterry& uniloc, int index) const {
		glUniform3f(uniloc.uPlightColors[index], this->m_color.r, this->m_color.g, this->m_color.b);
		glUniform3f(uniloc.uPlightPoses[index], mPos.x, mPos.y, mPos.z);
		glUniform1f(uniloc.uPlightMaxDists[index], mMaxDistance);
	}

}