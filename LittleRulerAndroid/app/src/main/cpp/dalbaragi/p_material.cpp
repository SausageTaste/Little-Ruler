#include "p_material.h"


namespace dal {

	Material::Material(void)
		: mShininess(32.0f),
		mSpecularStrength(1.0f),
		mTexScaleX(1.0f), mTexScaleY(1.0f),
		mDiffuseColor(1.0, 1.0, 1.0),
		mDiffuseMap(TextureMaster::getNullTex())
	{

	}

	/*
	std::string mName;

		float mShininess;
		float mSpecularStrength;
		float mTexScaleX, mTexScaleY;
		glm::vec3 mDiffuseColor;
	*/

	Material::Material(const Material& other)
	:	mName(other.mName),
		mShininess(other.mShininess),
		mSpecularStrength(other.mSpecularStrength),
		mTexScaleX(other.mTexScaleX),
		mTexScaleY(other.mTexScaleY),
		mDiffuseColor(other.mDiffuseColor)
	{

	}

	Material& Material::operator=(const Material& other) {
		this->mName = other.mName;
		this->mShininess = other.mShininess;
		this->mSpecularStrength = other.mSpecularStrength;
		this->mTexScaleX = other.mTexScaleX;
		this->mTexScaleY = other.mTexScaleY;
		this->mDiffuseColor = other.mDiffuseColor;

		return *this;
	}

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