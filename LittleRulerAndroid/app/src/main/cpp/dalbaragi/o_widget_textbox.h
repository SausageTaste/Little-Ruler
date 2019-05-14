#pragma once

#include <array>
#include <string>
#include <glm/glm.hpp>

#include "o_widget_primitive.h"
#include "s_event.h"
#include "o_text_cache.h"


namespace dal {

	class LineEdit : public iKeyboardListener {

	private:
		QuadPrimitive mMainBox;
		QuadPrimitive mCharDrawer;

		std::string mText;
		float mScale;

	public:
		LineEdit(void);
		virtual void give(const char* const str) override;
		void onReturn(void);

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
		bool append(const char* const str) {
			const auto len = std::strlen(str);
			const auto remaining = this->getReserved() - this->getSize();

			if (len > remaining) {
				LoggerGod::getinst().putError("StringBuffer is full.");
				return false;
			}

			std::memcpy(&this->m_buffer[this->m_topIndex], str, len);
			this->m_topIndex += len;
			return true;
		}

		unsigned int getSize(void) const {
			return this->m_topIndex;
		}

		unsigned int getReserved(void) const {
			return this->m_buffer.size();
		}

	};


	class TextBox {

	private:
		QuadPrimitive m_mainBox;
		StringBuffer* m_strBuffer = nullptr;

	public:
		StringBuffer* setStrBuf(StringBuffer* const strBuf);

	};

}