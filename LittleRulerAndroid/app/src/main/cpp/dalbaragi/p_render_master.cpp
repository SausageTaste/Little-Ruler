#include "p_render_master.h"

#include <string>
#include <vector>
#include <memory>

#include <glm/gtc/matrix_transform.hpp>

#include "u_fileclass.h"
#include "s_logger_god.h"
#include "s_scripting.h"
#include "o_widget_texview.h"


using namespace std::string_literals;


// Main Framebuffer
namespace dal {

	RenderMaster::MainFramebuffer::MainFramebuffer(void) {
		// Establish framebuffer
		{
			glGenFramebuffers(1, &m_mainFbuf);
			glBindFramebuffer(GL_FRAMEBUFFER, this->m_mainFbuf);

			glPixelStorei(GL_UNPACK_ALIGNMENT, 0);

			glGenTextures(1, &this->m_colorMap);
			glBindTexture(GL_TEXTURE_2D, this->m_colorMap);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->m_bufWidth, this->m_bufHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->m_colorMap, 0);
			glBindTexture(GL_TEXTURE_2D, 0);

			glGenRenderbuffers(1, &this->m_mainRenderbuf);
			glBindRenderbuffer(GL_RENDERBUFFER, this->m_mainRenderbuf);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, this->m_bufWidth, this->m_bufHeight);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, this->m_mainRenderbuf);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
				dalAbort("Failed to create framebuffer.");
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		// Establish vbo for fbuffer
		{
			glGenVertexArrays(1, &this->m_vbo);
			if ( this->m_vbo <= 0) dalAbort("Failed gen vertex array.");
			glGenBuffers(1, &this->m_vertexArr);
			if (m_vertexArr <= 0) dalAbort("Failed to gen a vertex buffer.");
			glGenBuffers(1, &this->m_texcoordArr);
			if ( this->m_texcoordArr <= 0) dalAbort("Failed to gen a texture coordinate buffer.");

			glBindVertexArray(this->m_vbo);

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

				glBindBuffer(GL_ARRAY_BUFFER, this->m_vertexArr);
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

				glBindBuffer(GL_ARRAY_BUFFER, this->m_texcoordArr);
				glBufferData(GL_ARRAY_BUFFER, size, texCoords, GL_STATIC_DRAW);

				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
			}

			glBindVertexArray(0);
		}

		this->m_tex = new Texture(this->m_colorMap);
	}

	RenderMaster::MainFramebuffer::~MainFramebuffer(void) {
		glDeleteBuffers(1, &this->m_vertexArr);
		glDeleteBuffers(1, &this->m_texcoordArr);
		glDeleteVertexArrays(1, &this->m_vbo);

		glDeleteRenderbuffers(1, &this->m_mainRenderbuf);
		glDeleteTextures(1, &this->m_colorMap);
		glDeleteFramebuffers(1, &this->m_mainFbuf);

		glDeleteFramebuffers(1, &m_mainFbuf);
	}

	void RenderMaster::MainFramebuffer::setRenderScale(float v, unsigned int win_width, unsigned int win_height) {
		m_renderScale = v;
		auto w = static_cast<unsigned int>(float(win_width) * v);
		auto h = static_cast<unsigned int>(float(win_height) * v);
		this->resizeFbuffer(w, h);
	}

	void RenderMaster::MainFramebuffer::resizeFbuffer(unsigned int newWin_width, unsigned int newWin_height) {
		m_bufWidth = (unsigned int)(float(newWin_width) * m_renderScale);
		m_bufHeight = (unsigned int)(float(newWin_height) * m_renderScale);

		glBindTexture(GL_TEXTURE_2D, m_colorMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_bufWidth, m_bufHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindRenderbuffer(GL_RENDERBUFFER, m_mainRenderbuf);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_bufWidth, m_bufHeight);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}

	void RenderMaster::MainFramebuffer::startRenderOn(void) {
		glBindFramebuffer(GL_FRAMEBUFFER, m_mainFbuf);
		glViewport(0, 0, m_bufWidth, m_bufHeight);
	}

	void RenderMaster::MainFramebuffer::renderOnScreen(const UnilocFScreen& uniloc) {
		glBindVertexArray(m_vbo);

		this->m_tex->sendUniform(uniloc.uTexture, 0, 0);

		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	Texture* RenderMaster::MainFramebuffer::getTex(void) {
		return this->m_tex;
	}

}


// Render Master
namespace dal {

	RenderMaster::RenderMaster(void)
		: m_scene(m_resMas),
		m_overlayMas(m_resMas, m_shader),
		m_winWidth(512), m_winHeight(512),
		m_projectMat(1.0)
	{
		// Lights
		{
			m_dlight1.m_color = { 0.7, 0.7, 0.7 };
			m_dlight1.mDirection = { -1.8, -1.0, 2.0 };
		}

		// OpenGL global switch
		{
			glClearColor(0.6f, 0.6f, 0.9f, 1.0f);
		}

		// Camera
		{
			this->m_camera.setPos(-1.25170159, 1.17569, 5.520179);
			this->m_camera.setViewPlane(0.9736287, -0.307451);
		}

		// Overlay
		{
			{
				auto t = new TextureView(nullptr, this->m_scene.getWater("test_level", 0)->m_fbuffer.getReflectionTexture());
				t->setPosX(10);
				t->setPosY(30);
				t->setWidth(256);
				t->setHeight(256);
				t->setPauseOnly(false);
				this->m_overlayMas.addWidget(t);
			}

			{
				auto t = new TextureView(nullptr, this->m_scene.getWater("test_level", 0)->m_fbuffer.getRefractionTexture());
				t->setPosX(10);
				t->setPosY(300);
				t->setWidth(256);
				t->setHeight(256);
				t->setPauseOnly(false);
				this->m_overlayMas.addWidget(t);
			}
		}

		// Misc
		{
			mHandlerName = "RenderMaster"s;
			EventGod::getinst().registerHandler(this, EventType::window_resize);

			float radio = static_cast<float>(m_winWidth) / static_cast<float>(m_winHeight);
			this->m_projectMat = glm::perspective(glm::radians(90.0f), radio, 0.01f, 100.0f);

			script::init_renderMas(this);
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

			m_dlight1.startRenderShadowmap(this->m_shader.getDepthMp());
			
			m_scene.renderDepthMp(this->m_shader.getDepthMp());

			m_dlight1.finishRenderShadowmap();
		}

		// Render to water framebuffer
		{
#ifdef _WIN32
			glEnable(GL_CLIP_DISTANCE0);
#endif
			this->m_shader.useGeneral();
			auto& unilocGeneral = this->m_shader.getGeneral();

			glUniform1i(unilocGeneral.u_doClip, 1);

			glUniformMatrix4fv(unilocGeneral.uProjectMat, 1, GL_FALSE, &m_projectMat[0][0]);

			glUniform3f(unilocGeneral.uBaseAmbient, 0.3f, 0.3f, 0.3f);

			// Lights

			this->m_dlight1.sendUniform(unilocGeneral, 0);
			glUniform1i(unilocGeneral.uDlightCount, 1);

			// Render meshes
			
			this->m_scene.renderGeneral_onWater(unilocGeneral, this->m_camera);
		}

		this->m_fbuffer.startRenderOn();
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
#ifdef _WIN32
		glDisable(GL_CLIP_DISTANCE0);
#endif

		// Render to framebuffer 
		{
			//this->m_shader.useGeneral();
			auto& unilocGeneral = this->m_shader.getGeneral();

			

			glUniformMatrix4fv(unilocGeneral.uProjectMat, 1, GL_FALSE, &m_projectMat[0][0]);
			
			glUniform1i(unilocGeneral.u_doClip, 0);

			const auto viewMat = this->m_camera.makeViewMat();
			glUniformMatrix4fv(unilocGeneral.uViewMat, 1, GL_FALSE, &viewMat[0][0]);

			const auto viewPos = this->m_camera.getPos();
			glUniform3f(unilocGeneral.uViewPos, viewPos.x, viewPos.y, viewPos.z);

			// Render meshes

			this->m_scene.renderGeneral(unilocGeneral);
		}

		// Render water to framebuffer

		{
			this->m_shader.useWaterry();
			auto& unilocWaterry = this->m_shader.getWaterry();

			glUniformMatrix4fv(unilocWaterry.uProjectMat, 1, GL_FALSE, &m_projectMat[0][0]);

			const auto viewMat = this->m_camera.makeViewMat();
			glUniformMatrix4fv(unilocWaterry.uViewMat, 1, GL_FALSE, &viewMat[0][0]);

			const auto viewPos = this->m_camera.getPos();
			glUniform3f(unilocWaterry.uViewPos, viewPos.x, viewPos.y, viewPos.z);

			glUniform3f(unilocWaterry.uBaseAmbient, 0.3f, 0.3f, 0.3f);

			// Lights

			m_dlight1.sendUniform(unilocWaterry, 0);
			glUniform1i(unilocWaterry.uDlightCount, 1);

			// Render meshes

			this->m_scene.renderWaterry(unilocWaterry);
		}

		// Render framebuffer to quad 
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, m_winWidth, m_winHeight);

			this->m_shader.useFScreen();
			this->m_fbuffer.renderOnScreen(this->m_shader.getFScreen());
		}

		this->m_overlayMas.render();
	}

	void RenderMaster::setRenderScale(float v) {
		this->m_fbuffer.setRenderScale(v, m_winWidth, m_winHeight);
	}

	void RenderMaster::onEvent(const EventStatic& e) {
		if (e.type == EventType::window_resize) {
			m_winWidth = static_cast<unsigned int>(e.intArg1);
			m_winHeight = static_cast<unsigned int>(e.intArg2);

			this->resizeFbuffer(m_winWidth, m_winHeight);
			this->m_scene.onResize(m_winWidth, m_winHeight);
		}
	}

	void RenderMaster::resizeFbuffer(unsigned int w, unsigned int h) {
		float radio = static_cast<float>(w) / static_cast<float>(h);
		this->m_projectMat = glm::perspective(glm::radians(90.0f), radio, 0.01f, 100.0f);
		
		this->m_fbuffer.resizeFbuffer(w, h);
	}

}