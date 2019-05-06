#include "p_material.h"


namespace dal {

	void Material::setTexScale(float x, float y) {
		if (x != 0.0f) mTexScaleX = x;
		if (y != 0.0f) mTexScaleY = y;
	}

	void Material::setDiffuseMap(dal::TextureHandle_ptr tex) {
		this->mDiffuseMap = tex;
	}

	void Material::setName(const char* const name) {
		mName = name;
	}

	void Material::sendUniform(const dal::UnilocGeneral& uniloc) const {
		glUniform1f(uniloc.uShininess, mShininess);
		glUniform1f(uniloc.uSpecularStrength, mSpecularStrength);

		glUniform1f(uniloc.uTexScaleX, mTexScaleX);
		glUniform1f(uniloc.uTexScaleY, mTexScaleY);

		glUniform3f(uniloc.uDiffuseColor, mDiffuseColor.x, mDiffuseColor.y, mDiffuseColor.z);

		// Diffuse map
		this->mDiffuseMap->sendUniform(uniloc.uDiffuseMap, uniloc.uHasDiffuseMap, 0);
	}

}