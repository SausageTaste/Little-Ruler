#pragma once

#include <glm/glm.hpp>

#include "p_resource.h"


namespace dal {

	struct QuadInfo {
		glm::vec2 p1, p2;
	};


	class IClickable {

	public:
		virtual ~IClickable(void) = default;
		virtual void onClick(const float x, const float y) = 0;

	};


	class IKeyInputTaker {

	public:
		virtual ~IKeyInputTaker(void) = default;
		virtual void onKeyInput(const char c) = 0;

	};


	class RenderableOverlay {

	public:
		virtual ~RenderableOverlay(void) = default;
		virtual void renderOverlay(const UnilocOverlay& uniloc) = 0;

	};


	class ScreenQuad {

	private:
		float m_xPos, m_yPos, m_width, m_height;
		QuadInfo m_deviceSpace;

	public:
		float getPosX(void) const { return m_xPos; }
		float getPosY(void) const { return m_yPos; }
		float getWidth(void) const { return m_width; }
		float getHeight(void) const { return m_height; }

		void setPosX(const float v);
		void setPosY(const float v);
		void setWidth(const float v);
		void setHeight(const float v);
		
		const QuadInfo& getDeviceSpace(void) const;

	private:
		void makeDeviceSpace(void);

	};


	class QuadRenderer : public ScreenQuad {

	private:
		glm::vec4 m_color{ 0, 0, 0, 1 };
		TextureHandle2 m_diffuseMap, m_maskMap;

	public:
		void setColor(const float r, const float g, const float b, const float a);
		void setDiffuseMap(const TextureHandle2& tex) { m_diffuseMap = tex; }
		void setMaskMap(const TextureHandle2& tex) { m_maskMap = tex; }

		void renderQuad(const UnilocOverlay& uniloc, const QuadInfo& devSpc);

	};

}