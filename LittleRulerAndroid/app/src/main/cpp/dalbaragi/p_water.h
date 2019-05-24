#pragma once

#include <glm/glm.hpp>

#include "p_dalopengl.h"
#include "p_meshStatic.h"
#include "p_uniloc.h"
#include "p_resource.h"
#include "u_timer.h"


namespace dal {

	class WaterFramebuffer {

	private:
		GLuint m_reflectionFrameBuffer;
		GLuint m_reflectionTexture;
		GLuint m_reflectionDepthBuffer;

		GLuint m_refractionFrameBuffer;
		GLuint m_refractionTexture;
		GLuint m_refractionDepthTexture;

		float m_winWidth = 0, m_winHeight = 0;
		float m_reflecScale = 0.7f, m_refracScale = 0.7f;

	public:
		WaterFramebuffer(const unsigned int winWidth, const unsigned int winHeight);
		~WaterFramebuffer(void);

		void bindReflectionFrameBuffer(void);
		void bindRefractionFrameBuffer(void);

		GLuint getReflectionTexture(void);
		GLuint getRefractionTexture(void);
		GLuint getRefractionDepthTexture(void);

		void resizeFbuffer(const unsigned int winWidth, const unsigned int winHeight);

	};

	
	class WaterRenderer {
	
	private:
		MeshStatic m_mesh;
		Material m_material;
		float m_height = 0.0f;
		float m_moveFactor = 0.0f;
		float m_moveSpeed = 0.03;
		Timer m_localTimer;

	public:
		WaterFramebuffer m_fbuffer;
		Texture m_reflectionTex;
		Texture m_refractionTex;

	public:
		WaterRenderer(const glm::vec3& pos, const glm::vec2& size);
		void renderWaterry(const UnilocWaterry& uniloc);
		float getHeight(void) const;
	
	};

}