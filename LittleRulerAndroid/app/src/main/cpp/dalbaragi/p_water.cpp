#include "p_water.h"

#include <array>

#include "s_logger_god.h"


namespace {

	constexpr int REFLECTION_WIDTH = 320;
	constexpr int REFLECTION_HEIGHT = 180;

	constexpr int REFRACTION_WIDTH = 1280;
	constexpr int REFRACTION_HEIGHT = 720;

}


namespace {

	GLuint genFramebuffer(void) {
		GLuint fbuffer;

		glGenFramebuffers(1, &fbuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, fbuffer);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		return fbuffer;
	}

	GLuint genTextureAttachment(const int width, const int height) {
		GLuint texture;

		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 512, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);

		return texture;
	}

	GLuint genDepthTextureAttachment(const int width, const int height) {
		GLuint texture;

		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture, 0);

		return texture;
	}

	GLuint genDepthBufferAttachment(const int width, const int height) {
		GLuint depthBuffer;

		glGenRenderbuffers(1, &depthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

		return depthBuffer;
	}

	void bindFrameBuffer(const GLuint frameBuffer, const int width, const int height) {
		glBindTexture(GL_TEXTURE_2D, 0);  //To make sure the texture isn't bound
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
		glViewport(0, 0, width, height);
	}

}


namespace dal {

	WaterFramebuffer::WaterFramebuffer(void) {
		{
			this->m_reflectionFrameBuffer = genFramebuffer();
			this->m_reflectionTexture = genTextureAttachment(REFLECTION_WIDTH, REFLECTION_HEIGHT);
			this->m_reflectionDepthBuffer = genDepthBufferAttachment(REFLECTION_WIDTH, REFLECTION_HEIGHT);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		
		{
			this->m_refractionFrameBuffer = genFramebuffer();
			this->m_refractionTexture = genTextureAttachment(REFRACTION_WIDTH, REFRACTION_HEIGHT);
			this->m_refractionDepthTexture = genDepthBufferAttachment(REFRACTION_WIDTH, REFRACTION_HEIGHT);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	WaterFramebuffer::~WaterFramebuffer(void) {
		glDeleteFramebuffers(1, &this->m_reflectionFrameBuffer);
		glDeleteTextures(1, &this->m_reflectionTexture);
		glDeleteRenderbuffers(1, &this->m_reflectionDepthBuffer);
		glDeleteFramebuffers(1, &this->m_refractionFrameBuffer);
		glDeleteTextures(1, &this->m_refractionTexture);
		glDeleteTextures(1, &this->m_refractionDepthTexture);
	}

	void WaterFramebuffer::bindReflectionFrameBuffer(void) {  //call before rendering to this FBO
		bindFrameBuffer(this->m_reflectionFrameBuffer, REFLECTION_WIDTH, REFLECTION_HEIGHT);
	}

	void WaterFramebuffer::bindRefractionFrameBuffer(void) {  //call before rendering to this FBO
		bindFrameBuffer(this->m_refractionFrameBuffer, REFRACTION_WIDTH, REFRACTION_HEIGHT);
	}

	GLuint WaterFramebuffer::getReflectionTexture(void) {  //get the resulting texture
		return this->m_reflectionTexture;
	}

	GLuint WaterFramebuffer::getRefractionTexture(void) {  //get the resulting texture
		return this->m_refractionTexture;
	}

	GLuint WaterFramebuffer::getRefractionDepthTexture(void) {  //get the resulting depth texture
		return this->m_refractionDepthTexture;
	}

}


namespace dal {

	WaterRenderer::WaterRenderer(const glm::vec3& pos, const glm::vec2& size)
		: m_tex(m_fbuffer.getReflectionTexture())
	{
		std::array<float, 18> vertices{
			pos.x,          pos.y, pos.z,
			pos.x,          pos.y, pos.z + size.y,
			pos.x + size.x, pos.y, pos.z + size.y,
			pos.x,          pos.y, pos.z,
			pos.x + size.x, pos.y, pos.z + size.y,
			pos.x + size.x, pos.y, pos.z
		};
		std::array<float, 12> texcoords{
			0, 1,
			0, 0,
			1, 0,
			0, 1,
			1, 0,
			1, 1
		};
		std::array<float, 18> normals{
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0
		};

		this->m_mesh.buildData(
			vertices.data(), vertices.size(),
			texcoords.data(), texcoords.size(),
			normals.data(), normals.size()
		);

		this->m_material.m_diffuseColor = { 0, 0, 1 };
	}

	void WaterRenderer::renderWaterry(const UnilocWaterry& uniloc) {
		this->m_material.sendUniform(uniloc);
		const glm::mat4 mat{ 1.0f };
		glUniformMatrix4fv(uniloc.uModelMat, 1, GL_FALSE, &mat[0][0]);
		this->m_mesh.draw();
	}

}