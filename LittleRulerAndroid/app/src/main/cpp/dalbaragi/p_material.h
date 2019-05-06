#pragma once

#include <string>

#include <glm/glm.hpp>

#include "p_texture.h"
#include "p_uniloc.h"


namespace dal {

	class Material {

	public:
		std::string mName;

		float mShininess = 32.0f;
		float mSpecularStrength = 1.0f;
		float mTexScaleX = 1.0f, mTexScaleY = 1.0f;
		glm::vec3 mDiffuseColor{ 1.0f, 1.0f, 1.0f };
		
	private:
		TextureHandle_ptr mDiffuseMap = TextureMaster::getNullTex();

	public:
		// If paremeter value is 0, old value remains.
		void setTexScale(float x, float y);
		void setDiffuseMap(TextureHandle_ptr tex);
		void setName(const char* const name);

		void sendUniform(const UnilocGeneral& uniloc) const;

	};

}