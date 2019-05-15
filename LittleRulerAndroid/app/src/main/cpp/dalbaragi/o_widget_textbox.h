#pragma once

#include <array>
#include <string>
#include <glm/glm.hpp>

#include "o_widget_primitive.h"
#include "s_event.h"
#include "o_text_cache.h"
#include "o_widget_base.h"
#include "s_scripting.h"


namespace dal {

	class LineEdit : public IKeyInputTaker {

	private:
		QuadPrimitive mMainBox;
		QuadPrimitive mCharDrawer;

		std::string mText;

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


	class LineEdit2 : public Widget {

	private:
		std::string mText;

	public:
		LineEdit2(void);
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


	class TextStream : public LuaStdOutput {

	private:
		std::array<char, 1024> m_buffer;
		unsigned int m_topIndex = 0;

	public:
		virtual bool append(const char* const str) override;
		bool append(const std::string& str);
		const char* get(void);
		void clear(void);

		unsigned int getSize(void) const;
		unsigned int getReserved(void) const;

	private:
		bool append(const char* const ptr, const size_t size);

	};


	class TextBox : public Widget {

	private:
		TextStream* m_strBuffer = nullptr;
		std::string m_text;
		QuadRenderer m_quadRender;
		const CharMaskMapCache& m_asciiCache;

		int m_scroll = 0;

	public:
		virtual void onClick(const float x, const float y) override;

		explicit TextBox(const CharMaskMapCache& asciiCache);
		TextStream* setStrBuf(TextStream* const strBuf);
		virtual void renderOverlay(const UnilocOverlay& uniloc) override;

		void setColor(float r, float g, float b, float a) {
			this->m_quadRender.setColor(r, g, b, a);
		}

		int addScroll(int v);

	private:
		void fetchStream(void);

	};

}