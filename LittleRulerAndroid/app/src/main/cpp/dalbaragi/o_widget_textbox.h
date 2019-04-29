#pragma once

#include <string>
#include <glm/glm.hpp>

#include "o_widget_primitive.h"
#include "s_event.h"
#include "o_text_cache.h"


namespace dal {

	class TextBox : public iKeyboardListener {

	private:
		QuadPrimitive mMainBox;
		QuadPrimitive mCharDrawer;

		std::string mText;
		float mScale;

	public:
		TextBox(void);
		virtual void give(const char* const str) override;
		void onReturn(void);

		void renderOverlay(const CharMaskMapCache& asciiCache, const UnilocOverlay& uniloc);
		void onResize(void);

		void setPos(float x, float y);
		void setSize(float w, float h);
		void setText(const char* const t);

		bool isInside(const glm::vec2& p) const;

	};

}