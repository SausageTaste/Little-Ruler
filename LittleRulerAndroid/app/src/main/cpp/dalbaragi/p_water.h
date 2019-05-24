#pragma once

#include <glm/glm.hpp>

#include "p_dalopengl.h"
#include "p_meshStatic.h"
#include "p_uniloc.h"
#include "p_resource.h"


namespace dal {

	class WaterFramebuffer {

	private:
		GLuint m_reflectionFrameBuffer;
		GLuint m_reflectionTexture;
		GLuint m_reflectionDepthBuffer;

		GLuint m_refractionFrameBuffer;
		GLuint m_refractionTexture;
		GLuint m_refractionDepthTexture;

	public:
		WaterFramebuffer(void);
		~WaterFramebuffer(void);

		void bindReflectionFrameBuffer(void);
		void bindRefractionFrameBuffer(void);

		GLuint getReflectionTexture(void);
		GLuint getRefractionTexture(void);
		GLuint getRefractionDepthTexture(void);

	};

	
	class WaterRenderer {
	
	private:
		MeshStatic m_mesh;
		Material m_material;
		float m_height = 0.0f;

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