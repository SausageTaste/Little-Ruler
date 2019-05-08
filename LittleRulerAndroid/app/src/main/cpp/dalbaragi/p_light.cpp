#include "p_light.h"

#include <glm/gtc/matrix_transform.hpp>

#include "p_glglobal.h"


namespace {

	constexpr unsigned int DEPTHMAP_RES = 1024 * 4;

}


namespace dal {

	DepthmapForLights::DepthmapForLights(void)
		: width(DEPTHMAP_RES), height(DEPTHMAP_RES)
	{
		this->mDepthmap = ResourceMaster::getDepthMap(width, height);

		glGenFramebuffers(1, &mFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, mFBO); {
			const GLenum none = GL_NONE;
			glDrawBuffers(1, &none);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mDepthmap.getTex(), 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, mDepthmap.getTex()); {
				if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) throw -1;
			} glBindTexture(GL_TEXTURE_2D, 0);

			glClear(GL_DEPTH_BUFFER_BIT);
		} glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	GLuint DepthmapForLights::getTextureID(void) {
		return mDepthmap.getTex();
	}

	TextureHandle2 DepthmapForLights::getDepthMap(void) {
		return mDepthmap;
	}

	void DepthmapForLights::startRender(void) {
		glBindFramebuffer(GL_FRAMEBUFFER, mFBO);

		glViewport(0, 0, width, height);
		glClear(GL_DEPTH_BUFFER_BIT);

		GLSwitch::setFor_shadowmap();
	}

	void DepthmapForLights::finishRender(void) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}

}


namespace dal {

	DirectionalLight::DirectionalLight(void)
		: mDirection(-0.3, -1, -1),
		mColor(1, 1, 1),
		mHalfShadowEdgeSize(15.0f)
	{
		this->mDirection = glm::normalize(this->mDirection);
	}

	void DirectionalLight::sendUniform(const UnilocGeneral& uniloc, int index) {
		glUniform3f(uniloc.uDlightColors[index], mColor.r, mColor.g, mColor.b);
		glUniform3f(uniloc.uDlightDirecs[index], mDirection.x, mDirection.y, mDirection.z);

		auto projViewMat = this->makeProjViewMap();
		glUniformMatrix4fv(uniloc.uDlightProjViewMat[index], 1, GL_FALSE, &projViewMat[0][0]);

		glActiveTexture(GL_TEXTURE1 + static_cast<GLuint>(index));
		glBindTexture(GL_TEXTURE_2D, mShadowMap.getTextureID());
		glUniform1i(uniloc.uDlightDepthMap[index], 1 + index);
	}

	void DirectionalLight::startRenderShadowmap(const UnilocDepthmp& uniloc) {
		auto mat = this->makeProjViewMap();
		glUniformMatrix4fv(uniloc.uProjViewMat, 1, GL_FALSE, &mat[0][0]);
		mShadowMap.startRender();
	}

	void DirectionalLight::finishRenderShadowmap(void) {
		mShadowMap.finishRender();
	}

	glm::mat4 DirectionalLight::makeProjViewMap(void) {
		return glm::ortho(
			-mHalfShadowEdgeSize, mHalfShadowEdgeSize,
			-mHalfShadowEdgeSize, mHalfShadowEdgeSize,
			-mHalfShadowEdgeSize, mHalfShadowEdgeSize
		) * glm::lookAt(-mDirection, { 0, 0, 0 }, { 0,1,0 });
	}

	GLuint DirectionalLight::getShadowMapTexture(void) {
		return mShadowMap.getTextureID();
	}

	TextureHandle2 DirectionalLight::getShadowMap(void) {
		return mShadowMap.getDepthMap();
	}

}


namespace dal {

	PointLight::PointLight(void)
		: mPos(0, 0, 0),
		mColor(1, 1, 1),
		mMaxDistance(5.0f)
	{

	}

	void PointLight::sendUniform(const UnilocGeneral& uniloc, int index) {
		glUniform3f(uniloc.uPlightColors[index], mColor.r, mColor.g, mColor.b);
		glUniform3f(uniloc.uPlightPoses[index], mPos.x, mPos.y, mPos.z);
		glUniform1f(uniloc.uPlightMaxDists[index], mMaxDistance);
	}

}