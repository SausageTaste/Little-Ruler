#pragma once

#include <string>

#include <glm/glm.hpp>

#include "p_uniloc.h"
#include "p_resource.h"


namespace dal {

	class DepthmapForLights {

	private:
		GLuint mFBO = 0;
		unsigned int width = 0, height = 0;
		Texture* mDepthmap = nullptr;

	public:
		DepthmapForLights(void);
		~DepthmapForLights(void);

		DepthmapForLights(const DepthmapForLights&) = delete;
		DepthmapForLights& operator=(const DepthmapForLights&) = delete;

		DepthmapForLights(DepthmapForLights&& other) noexcept;
		DepthmapForLights& operator=(DepthmapForLights&&) noexcept;

		GLuint getTextureID(void);
		const Texture* getDepthMap(void) const;

		void startRender(void);
		void finishRender(void);

	};


	class ILight {

	public:
	    std::string m_name;
        glm::vec3 m_color{ 1.0f, 1.0f, 1.0f };

	};


	class DirectionalLight : public ILight {

	public:
		glm::vec3 mDirection{ -0.3f, -1.0f, -1.0f };  // This must be always normalized.
		float mHalfShadowEdgeSize = 25.0f;

		DepthmapForLights mShadowMap;

	public:
		DirectionalLight(void);

		void sendUniform(const UnilocGeneral& uniloc, int index) const;
		void sendUniform(const UnilocWaterry& uniloc, int index) const;
		void startRenderShadowmap(const UnilocDepthmp& uniloc);
		void finishRenderShadowmap(void);

		glm::mat4 makeProjViewMap(void) const;
		GLuint getShadowMapTexture(void);
		const Texture* getShadowMap(void);

	};


	class PointLight : public ILight {

	public:
		glm::vec3 mPos;
		float mMaxDistance = 5.0f;

	public:
		void sendUniform(const UnilocGeneral& uniloc, int index) const;
		void sendUniform(const UnilocWaterry& uniloc, int index) const;

	};

}