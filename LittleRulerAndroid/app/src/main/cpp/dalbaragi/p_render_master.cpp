#include "p_render_master.h"

#include <string>
#include <vector>
#include <memory>

#include <glm/gtc/matrix_transform.hpp>

#include "u_fileclass.h"
#include "s_logger_god.h"
#include "p_glglobal.h"


using namespace std;


namespace {

	glm::mat4 makeCameraViewMat(const glm::vec3& pos, const glm::vec2& euler) {
		auto viewMat = glm::mat4(1.0f);

		viewMat = glm::rotate(viewMat, euler.y, glm::vec3(-1.0f, 0.0f, 0.0f));
		viewMat = glm::rotate(viewMat, euler.x, glm::vec3(0.0f, 1.0f, 0.0f));
		viewMat = glm::translate(viewMat, glm::vec3(-pos.x, -pos.y, -pos.z));

		return viewMat;
	}

}


namespace dal {

	RenderMaster::RenderMaster(void)
	:	mCameraPos(0.0, 0.0, 5.0),
		mCameraViewDir(0.0, 0.0),
		m_scene(m_texMas),
		m_overlayMas(m_texMas),
		mWidWidth(0), mWidHeight(0),
		mFbufWidth(10), mFbufHeight(10),
		mRenderScale(0.9f),
		mProjectMat(1.0),
		mShaderGeneral("mShaderGeneral"),
		mShaderFScreen("mShaderFScreen"),
		mShaderDepthmp("mShaderDepthmp"),
		mDlight1(&m_texMas)
	{
		/* Establish framebuffer */ {
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
				throw -1;
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		/* Establish vbo for fbuffer */ {
			glGenVertexArrays(1, &mMainVBO);
			if (mMainVBO <= 0) throw -1;
			glGenBuffers(1, &mMainBufVertices);
			if (mMainBufVertices <= 0) throw -1;
			glGenBuffers(1, &mMainBufTexCoords);
			if (mMainBufTexCoords <= 0) throw -1;

			glBindVertexArray(mMainVBO);

			/* Vertices */ {
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

			/* TexCoords */ {
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

		/* Compile shaders general */ {
			string vertSrc, fragSrc;
			file::getAsset_text("glsl/general_v.glsl", &vertSrc);
			file::getAsset_text("glsl/general_f.glsl", &fragSrc);

			auto verShader = compileShader(ShaderType::VERTEX, vertSrc.c_str());
			auto fragShader = compileShader(ShaderType::FRAGMENT, fragSrc.c_str());

			this->mShaderGeneral.attachShader(verShader);
			this->mShaderGeneral.attachShader(fragShader);
			this->mShaderGeneral.link();
			this->mUnilocGeneral.init(this->mShaderGeneral);

			glDeleteShader(verShader);
			glDeleteShader(fragShader);
		}

		/* Compile shaders fill screen */ {
			string vertSrc, fragSrc;
			file::getAsset_text("glsl/fillscreen_v.glsl", &vertSrc);
			file::getAsset_text("glsl/fillscreen_f.glsl", &fragSrc);

			auto verShader = compileShader(ShaderType::VERTEX, vertSrc.c_str());
			auto fragShader = compileShader(ShaderType::FRAGMENT, fragSrc.c_str());

			this->mShaderFScreen.attachShader(verShader);
			this->mShaderFScreen.attachShader(fragShader);
			this->mShaderFScreen.link();
			this->mUnilocFScreen.init(this->mShaderFScreen);

			glDeleteShader(verShader);
			glDeleteShader(fragShader);
		}

		/* Compile shaders fill screen */ {
			string vertSrc, fragSrc;
			file::getAsset_text("glsl/depth_v.glsl", &vertSrc);
			file::getAsset_text("glsl/depth_f.glsl", &fragSrc);

			auto verShader = compileShader(ShaderType::VERTEX, vertSrc.c_str());
			auto fragShader = compileShader(ShaderType::FRAGMENT, fragSrc.c_str());

			this->mShaderDepthmp.attachShader(verShader);
			this->mShaderDepthmp.attachShader(fragShader);
			this->mShaderDepthmp.link();
			this->mUnilocDepthmp.init(this->mShaderDepthmp);

			glDeleteShader(verShader);
			glDeleteShader(fragShader);
		}

		/* Lights */ {
			mPlight1.mPos = { 0, 2, 3 };
			mPlight1.mMaxDistance = 20.0f;
			mPlight1.mColor = { 0.0, 0.0, 0.0 };

			mDlight1.mColor = { 0.7, 0.7, 0.7 };
			mDlight1.mDirection = { 0.3, -1.0, -2.0 };
		}

		/* Misc */ {
			mHandlerName = "RenderMaster"s;
			EventGod::getinst().registerHandler(this, EventType::window_resize);

			float radio = float(mFbufWidth) / float(mFbufHeight);
			this->mProjectMat = glm::perspective(glm::radians(90.0f), radio, 0.01f, 100.0f);

			GLSwitch::setOnlyOnce();
		}
	}

	RenderMaster::~RenderMaster(void) {
		glDeleteFramebuffers(1, &mMainFbuffer);

		EventGod::getinst().deregisterHandler(this, EventType::window_resize);
	}

	void RenderMaster::update(const float deltaTime) {
		m_texMas.update();
	}

	void RenderMaster::render(void) {
		/* Shadow map */ {
			this->mShaderDepthmp.use();

			mDlight1.startRenderShadowmap(mUnilocDepthmp);
			
			m_scene.renderDepthMp(mUnilocDepthmp);

			mDlight1.finishRenderShadowmap();
		}

		/* Render to framebuffer */ {
			glBindFramebuffer(GL_FRAMEBUFFER, mMainFbuffer);

			glViewport(0, 0, mFbufWidth, mFbufHeight);
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

			GLSwitch::setFor_generalRender();

			this->mShaderGeneral.use();

			const auto identityMat = glm::mat4(1.0f);

			glUniformMatrix4fv(this->mUnilocGeneral.uProjectMat, 1, GL_FALSE, &mProjectMat[0][0]);
			
			auto viewMat = makeCameraViewMat(mCameraPos, mCameraViewDir);
			glUniformMatrix4fv(this->mUnilocGeneral.uViewMat, 1, GL_FALSE, &viewMat[0][0]);

			glUniformMatrix4fv(this->mUnilocGeneral.uModelMat, 1, GL_FALSE, &identityMat[0][0]);

			glUniform3f(mUnilocGeneral.uViewPos, mCameraPos.x, mCameraPos.y, mCameraPos.z);
			glUniform3f(mUnilocGeneral.uBaseAmbient, 0.3f, 0.3f, 0.3f);

			// Lights

			mDlight1.sendUniform(mUnilocGeneral, 0);
			glUniform1i(mUnilocGeneral.uDlightCount, 1);

			mPlight1.sendUniform(mUnilocGeneral, 0);
			glUniform1i(mUnilocGeneral.uPlightCount, 1);

			// Render meshes

			m_scene.renderGeneral(mUnilocGeneral);
		}

		/* Render framebuffer to quad */ {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glViewport(0, 0, mWidWidth, mWidHeight);
			//glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			GLSwitch::setFor_fillingScreen();

			mShaderFScreen.use();
			glBindVertexArray(mMainVBO);
			
			glBindTexture(GL_TEXTURE_2D, mMainFbuf_colorMap);
			//glBindTexture(GL_TEXTURE_2D, mDlight1.getShadowMap()->getTexID());
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		this->m_overlayMas.render();
	}

	void RenderMaster::setRenderScale(float v) {
		mRenderScale = v;
		auto w = (unsigned int)(float(mWidWidth) * v);
		auto h = (unsigned int)(float(mWidHeight) * v);
		this->resizeFbuffer(w, h);
	}

	void RenderMaster::onEvent(const EventStatic& e) {
		if (e.type == EventType::window_resize) {
			mWidWidth = (unsigned int)e.intArg1;
			mWidHeight = (unsigned int)e.intArg2;

			auto resizedWidth = (unsigned int)(float(mWidWidth) * mRenderScale);
			auto resizedHeight = (unsigned int)(float(mWidHeight) * mRenderScale);

			this->resizeFbuffer(resizedWidth, resizedHeight);
		}
	}

	void RenderMaster::resizeFbuffer(unsigned int w, unsigned int h) {
		mFbufWidth = w;
		mFbufHeight = h;

		float radio = float(w) / float(h);
		this->mProjectMat = glm::perspective(glm::radians(90.0f), radio, 0.01f, 100.0f);

		glBindTexture(GL_TEXTURE_2D, mMainFbuf_colorMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mFbufWidth, mFbufHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		glBindTexture(GL_TEXTURE_2D, 0);
		
		glBindRenderbuffer(GL_RENDERBUFFER, mMainRenderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, mFbufWidth, mFbufHeight);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		
	}

}