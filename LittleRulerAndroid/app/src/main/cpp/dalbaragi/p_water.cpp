#include "p_water.h"

#include <array>

#include "s_logger_god.h"
#include "u_fileclass.h"


namespace {

	dal::Texture* getDUDVMap(void) {
		static dal::Texture* tex = nullptr;

		if ( nullptr == tex ) {
			dal::loadedinfo::ImageFileData image;
			if ( !dal::futil::getRes_image("asset::waterDUDV.png", image) ) {
				dalAbort("Failed to load dudv map.");
			}
			assert(4 == image.m_pixSize);

			tex = new dal::Texture();
			if ( 4 == image.m_pixSize ) {
				tex->init_diffueMap(image.m_buf.data(), image.m_width, image.m_height);
			}
			else if ( 3 == image.m_pixSize ) {
				tex->init_diffueMap(image.m_buf.data(), image.m_width, image.m_height);
			}
			else {
				dalAbort("Wrong pixel size for dudv map");
			}
		}

		return tex;
	}

	dal::Texture* getWaterNormalMap(void) {
		static dal::Texture* tex = nullptr;

		if ( nullptr == tex ) {
			dal::loadedinfo::ImageFileData image;
			if ( !dal::futil::getRes_image("asset::matchingNormalMap.png", image) ) {
				dalAbort("Failed to load water normal map.");
			}
			assert(4 == image.m_pixSize);

			tex = new dal::Texture();
			if ( 4 == image.m_pixSize ) {
				tex->init_diffueMap(image.m_buf.data(), image.m_width, image.m_height);
			}
			else if ( 3 == image.m_pixSize ) {
				tex->init_diffueMap(image.m_buf.data(), image.m_width, image.m_height);
			}
			else {
				dalAbort("Wrong pixel size for normal normal map");
			}
		}

		return tex;
	}

}


namespace {

	GLuint genFramebuffer(void) {
		GLuint fbuffer;

		glGenFramebuffers(1, &fbuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, fbuffer);
		//glDrawBuffer(GL_COLOR_ATTACHMENT0);

		return fbuffer;
	}

	GLuint genTextureAttachment(const GLsizei width, const GLsizei height) {
		GLuint texture;

		glPixelStorei(GL_UNPACK_ALIGNMENT, 0);

		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
		//glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);

		return texture;
	}

	GLuint genDepthTextureAttachment(const GLsizei width, const GLsizei height) {
		GLuint texture;

		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		glClear(GL_DEPTH_BUFFER_BIT);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture, 0);

		return texture;
	}

	// This doesn't work for me. Now I use genDepthTextureAttachment instead.
	GLuint genDepthBufferAttachment(const int width, const int height) {
		GLuint depthBuffer;

		glGenRenderbuffers(1, &depthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
		{
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
		}
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		return depthBuffer;
	}

	void bindFrameBuffer(const GLuint frameBuffer, const int width, const int height) {
		glBindTexture(GL_TEXTURE_2D, 0);  //To make sure the texture isn't bound
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
		glViewport(0, 0, width, height);
	}

	bool checkFramebuffer(void) {
		const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		switch ( status ) {

		case GL_FRAMEBUFFER_COMPLETE:
			return true;

		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			dalError("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
			return false;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			dalError("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
			return false;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			dalError("GL_FRAMEBUFFER_UNSUPPORTED");
			return false;

		default:
			dalError("Unknown gl framebuffer error");
			return false;

		}

	}

}


namespace dal {

	void deleteFramebuffer(const GLuint fbo) {
		glDeleteFramebuffers(1, &fbo);
	}

	void deleteRenderbuffer(const GLuint rbo) {
		glDeleteRenderbuffers(1, &rbo);
	}

}


namespace dal {

	WaterFramebuffer::WaterFramebuffer(const unsigned int winWidth, const unsigned int winHeight)
		: m_winWidth(winWidth), m_winHeight(winHeight),
		m_reflecScale(0.5f), m_refracScale(0.5f)
	{
		const GLsizei REFLECTION_WIDTH  = static_cast<GLsizei>(this->m_winWidth  * this->m_reflecScale);
		const GLsizei REFLECTION_HEIGHT = static_cast<GLsizei>(this->m_winHeight * this->m_reflecScale);
		const GLsizei REFRACTION_WIDTH  = static_cast<GLsizei>(this->m_winWidth  * this->m_refracScale);
		const GLsizei REFRACTION_HEIGHT = static_cast<GLsizei>(this->m_winHeight * this->m_refracScale);

		{
			this->m_reflectionFrameBuffer.reset(genFramebuffer());
			this->m_reflectionTexture.initAttach_colorMap(REFLECTION_WIDTH, REFLECTION_HEIGHT);
			this->m_reflectionDepthBuffer.reset(genDepthBufferAttachment(REFLECTION_WIDTH, REFLECTION_HEIGHT));
		}

		if ( !checkFramebuffer() ) dalError("Framebuffer creation failed for reflection.");
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		
		{
			this->m_refractionFrameBuffer.reset(genFramebuffer());
			this->m_refractionTexture.initAttach_colorMap(REFRACTION_WIDTH, REFRACTION_HEIGHT);
			this->m_refractionDepthTexture.reset(genDepthBufferAttachment(REFRACTION_WIDTH, REFRACTION_HEIGHT));
		}

		if ( !checkFramebuffer() ) dalError("Framebuffer creation failed for reflection.");
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void WaterFramebuffer::bindReflectionFrameBuffer(void) {  //call before rendering to this FBO
		bindFrameBuffer(
			this->m_reflectionFrameBuffer.get(),
			static_cast<int>(this->m_winWidth  * this->m_reflecScale),
			static_cast<int>(this->m_winHeight  * this->m_reflecScale)
		);
	}

	void WaterFramebuffer::bindRefractionFrameBuffer(void) {  //call before rendering to this FBO
		bindFrameBuffer(this->m_refractionFrameBuffer.get(),
			static_cast<int>(this->m_winWidth * this->m_refracScale),
			static_cast<int>(this->m_winHeight * this->m_refracScale)
		);
	}

	Texture* WaterFramebuffer::getReflectionTexture(void) {  //get the resulting texture
		return &this->m_reflectionTexture;
	}

	Texture* WaterFramebuffer::getRefractionTexture(void) {  //get the resulting texture
		return &this->m_refractionTexture;
	}

	GLuint WaterFramebuffer::getRefractionDepthTexture(void) {  //get the resulting depth texture
		return this->m_refractionDepthTexture.get();
	}

	void WaterFramebuffer::resizeFbuffer(const unsigned int winWidth, const unsigned int winHeight) {
		this->m_winWidth = static_cast<float>(winWidth);
		this->m_winHeight = static_cast<float>(winHeight);

		const GLsizei bansa_width = static_cast<GLsizei>(this->m_winWidth  * this->m_reflecScale);
		const GLsizei bansa_height = static_cast<GLsizei>(this->m_winHeight * this->m_reflecScale);
		const GLsizei gooljul_width = static_cast<GLsizei>(this->m_winWidth  * this->m_refracScale);
		const GLsizei gooljul_height = static_cast<GLsizei>(this->m_winHeight * this->m_refracScale);

		{
			glBindTexture(GL_TEXTURE_2D, this->m_reflectionTexture.get());
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bansa_width, bansa_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		{
			glBindTexture(GL_TEXTURE_2D, this->m_refractionTexture.get());
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, gooljul_width, gooljul_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		{
			glBindRenderbuffer(GL_RENDERBUFFER, this->m_reflectionDepthBuffer.get());
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, bansa_width, bansa_height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
		}
		
		{
			glBindRenderbuffer(GL_RENDERBUFFER, this->m_refractionDepthTexture.get());
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, gooljul_width, gooljul_height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
		}
	}

}


namespace dal {

	WaterRenderer::WaterRenderer(const glm::vec3& pos, const glm::vec2& size)
		: m_height(pos.y),
		m_fbuffer(12, 12)
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
		this->m_material.m_specularStrength = 2.0f;
		this->m_material.m_shininess = 16.0;
	}

	void WaterRenderer::renderWaterry(const UnilocWaterry& uniloc) {
		const auto deltaTime = this->m_localTimer.check_getElapsed_capFPS();
		this->m_moveFactor += this->m_moveSpeed * deltaTime;
		this->m_moveFactor = fmod(this->m_moveFactor, 1.0f);
		glUniform1f(uniloc.u_dudvMoveFactor, this->m_moveFactor);

		this->m_material.sendUniform(uniloc);

		this->m_fbuffer.getReflectionTexture()->sendUniform(uniloc.u_bansaTex, 0, 4);
		this->m_fbuffer.getRefractionTexture()->sendUniform(uniloc.u_gooljulTex, 0, 5);
		getDUDVMap()->sendUniform(uniloc.u_dudvMap, 0, 6);
		getWaterNormalMap()->sendUniform(uniloc.u_normalMap, 0, 7);

		const glm::mat4 mat{ 1.0f };
		glUniformMatrix4fv(uniloc.uModelMat, 1, GL_FALSE, &mat[0][0]);
		this->m_mesh.draw();
	}

	float WaterRenderer::getHeight(void) const {
		return this->m_height;
	}

}