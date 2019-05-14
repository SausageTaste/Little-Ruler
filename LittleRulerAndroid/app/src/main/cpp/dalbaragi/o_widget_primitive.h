#pragma once

#include "glm/glm.hpp"

#include "p_uniloc.h"
#include "p_resource.h"


namespace dal {

	class QuadPrimitive {

	//////// vars ////////

	private:
		// Point in screen coordinate system. Left upper corner is (0, 0) and right below corner is (width, height).
		glm::vec2 mPointScr1;
		glm::vec2 mPointScr2;
		glm::vec4 mColor;

		// This is described as device space coordinate system which between -1 and 1.
		// These are gonna used directly to render.
		glm::vec2 mPointDev1;  
		glm::vec2 mPointDev2;

		TextureHandle2 mDiffuseMap;
		TextureHandle2 mMaskMap;

	//////// funcs ////////

	public:
		QuadPrimitive(void);
		void renderOverlay(const UnilocOverlay& uniloc);

		void moveCenterTo_screenCoord(float x, float y);
		void moveCornerTo_screenCoord(float x, float y);

		void setPointScrs(const float x1, const float y1, const float x2, const float y2);
		void setWidth(const float v);
		void setHeight(const float v);

		void setColor(const float r, const float g, const float b);
		void setColor(glm::vec3 v);
		void setTransparency(const float a);

		void setDiffuseMap(const TextureHandle2& tex);
		void setMaskMap(const TextureHandle2& tex);

		const glm::vec2& getPointScr1(void) const;
		const glm::vec2& getPointScr2(void) const;
		float getWidth(void) const;
		float getHeight(void) const;

		void convertScrIntoDev(void);

	};

}