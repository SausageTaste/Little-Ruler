#pragma once

#include <array>
#include <string>
#include <glm/glm.hpp>

#include "o_widget_primitive.h"
#include "s_event.h"
#include "o_text_cache.h"
#include "o_widget_base.h"


namespace dal {

	class LineEdit : public IKeyInputTaker {

	private:
		QuadPrimitive mMainBox;
		QuadPrimitive mCharDrawer;

		std::string mText;
		float mScale;

	public:
		LineEdit(void);
		void onReturn(void);
		void onKeyInput(const char c) override;
		void renderOverlay(const CharMaskMapCache& asciiCache, const UnilocOverlay& uniloc);
		void onResize(void);

		void setPos(float x, float y);
		void setSize(float w, float h);
		void setText(const char* const t);
		void setTextColor(const float r, const float g, const float b);

		bool isInside(const glm::vec2& p) const;

	};


	class StringBuffer {

	private:
		std::array<char, 1024> m_buffer;
		unsigned int m_topIndex = 0;

	public:
		bool append(const char* const str);

		unsigned int getSize(void) const;

		unsigned int getReserved(void) const;

	};


	class TextBox {

	private:
		QuadPrimitive m_mainBox;
		StringBuffer* m_strBuffer = nullptr;

	public:
		StringBuffer* setStrBuf(StringBuffer* const strBuf);

	};

}