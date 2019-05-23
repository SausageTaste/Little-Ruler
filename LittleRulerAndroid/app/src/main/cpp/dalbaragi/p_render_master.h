#pragma once

#include <list>

#include <glm/glm.hpp>


#include "s_event.h"

#include "p_shader_master.h"
#include "p_uniloc.h"
#include "p_meshStatic.h"
#include "p_light.h"
#include "o_overlay_master.h"
#include "p_dalopengl.h"
#include "p_scene.h"
#include "g_actor.h"


namespace dal {

	class RenderMaster : iEventHandler {

	private:
		class MainFramebuffer {
			unsigned int m_bufWidth = 256, m_bufHeight = 256;
			float m_renderScale = 1.0f;

			GLuint m_mainFbuf = 0;
			GLuint m_colorMap = 0;
			GLuint m_mainRenderbuf = 0;
			GLuint m_vbo = 0;
			GLuint m_vertexArr = 0;
			GLuint m_texcoordArr = 0;

		public:
			MainFramebuffer(void);
			~MainFramebuffer(void);

			void setRenderScale(float v, unsigned int widWidth, unsigned int widHeight);
			void resizeFbuffer(unsigned int w, unsigned int h);

			void startRenderOn(void);
			void renderOnScreen(void);

		};

	private:
		ShaderMaster m_shader;
		MainFramebuffer m_fbuffer;

	public:
		Camera m_camera;
		ResourceMaster m_resMas;
		SceneMaster m_scene;
		OverlayMaster m_overlayMas;

	private:
		unsigned int m_winWidth, m_winHeight;
		glm::mat4 m_projectMat;
		DirectionalLight m_dlight1;

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