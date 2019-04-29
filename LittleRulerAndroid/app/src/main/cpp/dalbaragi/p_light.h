#pragma once

#include <glm/glm.hpp>

#include "p_uniloc.h"
#include "p_texture.h"


namespace dal {

	class DepthmapForLights {

	private:
		GLuint mFBO;

		unsigned int width, height;

		TextureHandle_ptr mDepthmap;

	public:
		DepthmapForLights(TextureMaster* const texMaster);

		GLuint getTextureID(void);
		TextureHandle_ptr getDepthMap(void);

		void startRender(void);

		void finishRender(void);

	};


	class DirectionalLight {

	public:
		glm::vec3 mDirection;  // This must be always normalized.
		glm::vec3 mColor;

		DepthmapForLights mShadowMap;
		float mHalfShadowEdgeSize;

	public:
		DirectionalLight(TextureMaster* const texMaster);

		void sendUniform(const UnilocGeneral& uniloc, int index);

		void startRenderShadowmap(const UnilocDepthmp& uniloc);

		void finishRenderShadowmap(void);

		glm::mat4 makeProjViewMap(void);

		GLuint getShadowMapTexture(void);

		TextureHandle_ptr getShadowMap(void);

	};

	class PointLight {

	public:
		glm::vec3 mPos;
		glm::vec3 mColor;
		float mMaxDistance;

	public:
		PointLight(void);

		void sendUniform(const UnilocGeneral& uniloc, int index);

	};

}