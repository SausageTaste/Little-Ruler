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

	private:
		class ShaderMaster {

		private:
			ShaderProgram m_general;
			UnilocGeneral m_generalUniloc;

			ShaderProgram m_depthmap;
			UnilocDepthmp m_depthmapUniloc;

			ShaderProgram m_fscreen;
			UnilocFScreen m_fscreenUniloc;

		public:
			ShaderMaster(void);

			void useGeneral(void);
			void useDepthMp(void);
			void useFScreen(void);

			const UnilocGeneral& getGeneral(void) const;
			const UnilocDepthmp& getDepthMp(void) const;
			const UnilocFScreen& getFScreen(void) const;

		};

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

	public:
		glm::vec3 mCameraPos;
		glm::vec2 mCameraViewDir;

	private:
		ResourceMaster m_resMas;
		SceneMaster m_scene;
		ShaderMaster m_shader;
		MainFramebuffer m_fbuffer;

	public:
		OverlayMaster m_overlayMas;

	private:
		unsigned int m_winWidth, m_winHeight;

		glm::mat4 m_projectMat;

		DirectionalLight m_dlight1;
		PointLight m_plight1;

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