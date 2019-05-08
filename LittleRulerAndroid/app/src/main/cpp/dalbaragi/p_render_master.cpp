#include "p_render_master.h"

#include <string>
#include <vector>
#include <memory>

#include <glm/gtc/matrix_transform.hpp>

#include "u_fileclass.h"
#include "s_logger_god.h"
#include "p_glglobal.h"


using namespace std::string_literals;


namespace {

	glm::mat4 makeCameraViewMat(const glm::vec3& pos, const glm::vec2& euler) {
		auto viewMat = glm::mat4(1.0f);

		viewMat = glm::rotate(viewMat, euler.y, glm::vec3(-1.0f, 0.0f, 0.0f));
		viewMat = glm::rotate(viewMat, euler.x, glm::vec3(0.0f, 1.0f, 0.0f));
		viewMat = glm::translate(viewMat, glm::vec3(-pos.x, -pos.y, -pos.z));

		return viewMat;
	}

}

 // Shader master
namespace dal {

	RenderMaster::ShaderMaster::ShaderMaster(void)
	:	m_general("shader_general"),
		m_depthmap("shader_fscreen"),
		m_fscreen("shader_depthmap")
	{
		// Compile shaders general
		{
			std::string vertSrc, fragSrc;
			filec::getAsset_text("glsl/general_v.glsl", &vertSrc);
			filec::getAsset_text("glsl/general_f.glsl", &fragSrc);

			auto verShader = compileShader(ShaderType::VERTEX, vertSrc.c_str());
			auto fragShader = compileShader(ShaderType::FRAGMENT, fragSrc.c_str());

			this->m_general.attachShader(verShader);
			this->m_general.attachShader(fragShader);
			this->m_general.link();
			this->m_generalUniloc.init(this->m_general);

			glDeleteShader(verShader);
			glDeleteShader(fragShader);
		}

		// Compile shaders fill screen
		{
			std::string vertSrc, fragSrc;
			filec::getAsset_text("glsl/fillscreen_v.glsl", &vertSrc);
			filec::getAsset_text("glsl/fillscreen_f.glsl", &fragSrc);

			auto verShader = compileShader(ShaderType::VERTEX, vertSrc.c_str());
			auto fragShader = compileShader(ShaderType::FRAGMENT, fragSrc.c_str());

			this->m_fscreen.attachShader(verShader);
			this->m_fscreen.attachShader(fragShader);
			this->m_fscreen.link();
			this->m_fscreenUniloc.init(this->m_fscreen);

			glDeleteShader(verShader);
			glDeleteShader(fragShader);
		}

		// Compile shaders fill screen
		{
			std::string vertSrc, fragSrc;
			filec::getAsset_text("glsl/depth_v.glsl", &vertSrc);
			filec::getAsset_text("glsl/depth_f.glsl", &fragSrc);

			auto verShader = compileShader(ShaderType::VERTEX, vertSrc.c_str());
			auto fragShader = compileShader(ShaderType::FRAGMENT, fragSrc.c_str());

			this->m_depthmap.attachShader(verShader);
			this->m_depthmap.attachShader(fragShader);
			this->m_depthmap.link();
			this->m_depthmapUniloc.init(this->m_depthmap);

			glDeleteShader(verShader);
			glDeleteShader(fragShader);
		}
	}

	void RenderMaster::ShaderMaster::useGeneral(void) {
		this->m_general.use();
	}

	void RenderMaster::ShaderMaster::useDepthMp(void) {
		this->m_depthmap.use();
	}

	void RenderMaster::ShaderMaster::useFScreen(void) {
		this->m_fscreen.use();
	}

	const UnilocGeneral& RenderMaster::ShaderMaster::getGeneral(void) const {
		return this->m_generalUniloc;
	}

	const UnilocDepthmp& RenderMaster::ShaderMaster::getDepthMp(void) const {
		return this->m_depthmapUniloc;
	}

	const UnilocFScreen& RenderMaster::ShaderMaster::getFScreen(void) const {
		return this->m_fscreenUniloc;
	}

}


namespace dal {

	RenderMaster::MainFramebuffer::MainFramebuffer(void) {
		// Establish framebuffer
		{
			glGenFramebuffers(1, &mMainFbuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, mMainFbuffer);

			glPixelStorei(GL_UNPACK_ALIGNMENT, 0);

			glGenTextures(1, &mMainFbuf_colorMap);
			glBindTexture(GL_TEXTURE_2D, mMainFbuf_colorMap);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mFbufWidth, mFbufHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mMainFbuf_colorMap, 0);
			glBindTexture(GL_TEXTURE_2D, 0);

			glGenRenderbuffers(1, &mMainRenderbuffer);
			glBindRenderbuffer(GL_RENDERBUFFER, mMainRenderbuffer);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, mFbufWidth, mFbufHeight);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mMainRenderbuffer);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
				LoggerGod::getinst().putFatal("Failed to create framebuffer.");
				throw - 1;
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		// Establish vbo for fbuffer
		{
			glGenVertexArrays(1, &mMainVBO);
			if (mMainVBO <= 0) throw - 1;
			glGenBuffers(1, &mMainBufVertices);
			if (mMainBufVertices <= 0) throw - 1;
			glGenBuffers(1, &mMainBufTexCoords);
			if (mMainBufTexCoords <= 0) throw - 1;

			glBindVertexArray(mMainVBO);

			// Vertices
			{
				GLfloat vertices[12] = {
					-1,  1,
					-1, -1,
					 1, -1,
					-1,  1,
					 1, -1,
					 1,  1
				};
				auto size = 12 * sizeof(float);

				glBindBuffer(GL_ARRAY_BUFFER, this->mMainBufVertices);
				glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
			}

			// TexCoords
			{
				GLfloat texCoords[12] = {
					0, 1,
					0, 0,
					1, 0,
					0, 1,
					1, 0,
					1, 1
				};
				auto size = 12 * sizeof(float);

				glBindBuffer(GL_ARRAY_BUFFER, this->mMainBufTexCoords);
				glBufferData(GL_ARRAY_BUFFER, size, texCoords, GL_STATIC_DRAW);

				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
			}

			glBindVertexArray(0);
		}
	}

	RenderMaster::MainFramebuffer::~MainFramebuffer(void) {
		glDeleteFramebuffers(1, &mMainFbuffer);
	}

	void RenderMaster::MainFramebuffer::setRenderScale(float v, unsigned int win_width, unsigned int win_height) {
		mRenderScale = v;
		auto w = static_cast<unsigned int>(float(win_width) * v);
		auto h = static_cast<unsigned int>(float(win_height) * v);
		this->resizeFbuffer(w, h);
	}

	void RenderMaster::MainFramebuffer::resizeFbuffer(unsigned int newWin_width, unsigned int newWin_height) {
		mFbufWidth = (unsigned int)(float(newWin_width) * mRenderScale);
		mFbufHeight = (unsigned int)(float(newWin_height) * mRenderScale);

		glBindTexture(GL_TEXTURE_2D, mMainFbuf_colorMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mFbufWidth, mFbufHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindRenderbuffer(GL_RENDERBUFFER, mMainRenderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, mFbufWidth, mFbufHeight);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}

	void RenderMaster::MainFramebuffer::startRenderOn(void) {
		glBindFramebuffer(GL_FRAMEBUFFER, mMainFbuffer);
		glViewport(0, 0, mFbufWidth, mFbufHeight);
	}

	void RenderMaster::MainFramebuffer::renderOnScreen(void) {
		glBindVertexArray(mMainVBO);

		glBindTexture(GL_TEXTURE_2D, mMainFbuf_colorMap);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

}


namespace dal {

	RenderMaster::RenderMaster(void)
	:	mCameraPos(0.0, 0.0, 5.0),
		mCameraViewDir(0.0, 0.0),
		m_scene(m_resMas),
		m_overlayMas(m_resMas),
		mWidWidth(512), mWidHeight(512),
		mProjectMat(1.0)
	{
		// Lights
		{
			mPlight1.mPos = { 0, 2, 3 };
			mPlight1.mMaxDistance = 20.0f;
			mPlight1.mColor = { 0.0, 0.0, 0.0 };

			mDlight1.mColor = { 0.7, 0.7, 0.7 };
			mDlight1.mDirection = { 0.3, -1.0, -2.0 };
		}

		// Misc
		{
			mHandlerName = "RenderMaster"s;
			EventGod::getinst().registerHandler(this, EventType::window_resize);

			float radio = float(mWidWidth) / float(mWidHeight);
			this->mProjectMat = glm::perspective(glm::radians(90.0f), radio, 0.01f, 100.0f);

			GLSwitch::setOnlyOnce();
		}
	}

	RenderMaster::~RenderMaster(void) {
		EventGod::getinst().deregisterHandler(this, EventType::window_resize);
	}

	void RenderMaster::update(const float deltaTime) {

	}

	void RenderMaster::render(void) {
		// Shadow map
		{
			this->m_shader.useDepthMp();

			mDlight1.startRenderShadowmap(this->m_shader.getDepthMp());
			
			m_scene.renderDepthMp(this->m_shader.getDepthMp());

			mDlight1.finishRenderShadowmap();
		}

		// Render to framebuffer 
		{
			this->m_fbuffer.startRenderOn();
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
			GLSwitch::setFor_generalRender();

			this->m_shader.useGeneral();
			auto& unilocGeneral = this->m_shader.getGeneral();

			const auto identityMat = glm::mat4(1.0f);

			glUniformMatrix4fv(unilocGeneral.uProjectMat, 1, GL_FALSE, &mProjectMat[0][0]);
			
			auto viewMat = makeCameraViewMat(mCameraPos, mCameraViewDir);
			glUniformMatrix4fv(unilocGeneral.uViewMat, 1, GL_FALSE, &viewMat[0][0]);

			glUniformMatrix4fv(unilocGeneral.uModelMat, 1, GL_FALSE, &identityMat[0][0]);

			glUniform3f(unilocGeneral.uViewPos, mCameraPos.x, mCameraPos.y, mCameraPos.z);
			glUniform3f(unilocGeneral.uBaseAmbient, 0.3f, 0.3f, 0.3f);

			// Lights

			mDlight1.sendUniform(unilocGeneral, 0);
			glUniform1i(unilocGeneral.uDlightCount, 1);

			mPlight1.sendUniform(unilocGeneral, 0);
			glUniform1i(unilocGeneral.uPlightCount, 1);

			// Render meshes

			m_scene.renderGeneral(unilocGeneral);
		}

		// Render framebuffer to quad 
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glViewport(0, 0, mWidWidth, mWidHeight);
			//glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			GLSwitch::setFor_fillingScreen();

			this->m_shader.useFScreen();
			this->m_fbuffer.renderOnScreen();
		}

		this->m_overlayMas.render();
	}

	void RenderMaster::setRenderScale(float v) {
		this->m_fbuffer.setRenderScale(v, mWidWidth, mWidHeight);
	}

	void RenderMaster::onEvent(const EventStatic& e) {
		if (e.type == EventType::window_resize) {
			mWidWidth = (unsigned int)e.intArg1;
			mWidHeight = (unsigned int)e.intArg2;

			this->resizeFbuffer(mWidWidth, mWidHeight);
		}
	}

	void RenderMaster::resizeFbuffer(unsigned int w, unsigned int h) {
		float radio = float(w) / float(h);
		this->mProjectMat = glm::perspective(glm::radians(90.0f), radio, 0.01f, 100.0f);
		
		this->m_fbuffer.resizeFbuffer(w, h);
	}

}