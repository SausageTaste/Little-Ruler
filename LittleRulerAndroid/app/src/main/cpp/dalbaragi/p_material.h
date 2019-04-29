#pragma once

#include <string>

#include <glm/glm.hpp>

#include "p_texture.h"
#include "p_uniloc.h"


namespace dal {

	class Material {

	public:
		std::string mName;

		float mShininess;
		float mSpecularStrength;
		float mTexScaleX, mTexScaleY;
		glm::vec3 mDiffuseColor;
		
	private:
		TextureHandle_ptr mDiffuseMap = TextureMaster::getNullTex();

	public:
		Material(void);

		Material(const Material&);
		Material& operator=(const Material&);

		// If paremeter value is 0, old value remains.
		void setTexScale(float x, float y);
		void setDiffuseMap(TextureHandle_ptr tex);
		void setName(const char* const name);

		void sendUniform(const UnilocGeneral& uniloc) const;

	};

}