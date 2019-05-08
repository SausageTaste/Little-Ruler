#pragma once

#include <list>

#include <glm/glm.hpp>


#include "s_event.h"

#include "p_shader.h"
#include "p_uniloc.h"
#include "p_meshStatic.h"
#include "p_light.h"
#include "o_overlay_master.h"
#include "p_dalopengl.h"
#include "p_scene.h"


namespace dal {

	class RenderMaster : iEventHandler {

	public:
		glm::vec3 mCameraPos;
		glm::vec2 mCameraViewDir;

	private:
		ResourceMaster m_resMas;
		SceneMaster m_scene;

	public:
		OverlayMaster m_overlayMas;

	private:
		unsigned int mWidWidth, mWidHeight, mFbufWidth, mFbufHeight;
		GLuint mMainFbuffer;
		GLuint mMainFbuf_colorMap;
		GLuint mMainRenderbuffer;
		GLuint mMainVBO;
		GLuint mMainBufVertices;
		GLuint mMainBufTexCoords;

		float mRenderScale;

		glm::mat4 mProjectMat;

		// Shaders
		ShaderProgram mShaderGeneral;
		UnilocGeneral mUnilocGeneral;

		ShaderProgram mShaderFScreen;
		UnilocFScreen mUnilocFScreen;

		ShaderProgram mShaderDepthmp;
		UnilocDepthmp mUnilocDepthmp;

		DirectionalLight mDlight1;
		PointLight mPlight1;

	public:
		RenderMaster(void);
		virtual ~RenderMaster(void) override;

		void update(const float deltaTime);
		void render(void);
		void setRenderScale(float v);

		virtual void onEvent(const EventStatic& e) override;

	private:
		void resizeFbuffer(unsigned int w, unsigned int h);

	};

}