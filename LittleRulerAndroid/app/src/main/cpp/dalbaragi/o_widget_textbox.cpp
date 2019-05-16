#include "o_widget_textbox.h"

#include <cstring>
#include <string>

#include "s_logger_god.h"
#include "m_collision2d.h"
#include "s_scripting.h"


using namespace std::string_literals;


namespace {

	dal::QuadRenderer g_charDrawer;

	constexpr float g_darkTheme = 30.0f / 255.0f;

}


namespace dal {

	LineEdit2::LineEdit2(const CharMaskMapCache& asciiCache)
		: m_asciiCache(asciiCache)
	{
		this->setPosX(10.0f);
		this->setPosY(40.0f);
		this->setWidth(400.0f);
		this->setHeight(20.0f);

		this->m_quadRender.setColor(g_darkTheme, g_darkTheme, g_darkTheme, 1.0f);
	}

	void LineEdit2::onKeyInput(const char c) {
		switch (c) {

		case '\n':
			this->onReturn();
			break;
		case '\b':
			if (mText.empty()) break;
			mText.pop_back();
			break;
		case '\t':
			mText += "    ";  // 4 whitespaces. 
			break;
		case '\0':
			return;
		default:
			mText += c;
			break;

		}
	}

	void LineEdit2::onReturn(void) {
		Lua::getinst().doString(mText.c_str());
		mText.clear();
	}

	void LineEdit2::renderOverlay(const UnilocOverlay& uniloc) {
		this->m_quadRender.renderQuad(uniloc, this->getDeviceSpace());

		const auto screenInfo = this->makeScreenSpace();

		float xAdvance = screenInfo.p1.x;
		const float boxHeight = screenInfo.p2.y - screenInfo.p1.y;
		const float yHeight = screenInfo.p2.y - boxHeight / 4.0f;

		for (auto c : mText) {
			auto& charac = this->m_asciiCache.at((unsigned int)c);

			QuadInfo charQuad;

			charQuad.p1.x = xAdvance + charac.bearing.x;
			charQuad.p1.y = yHeight - charac.bearing.y;

			charQuad.p2.x = charQuad.p1.x + charac.size.x;
			charQuad.p2.y = charQuad.p1.y + charac.size.y;

			g_charDrawer.setMaskMap(charac.tex);
			g_charDrawer.renderQuad(uniloc, charQuad.screen2device());

			xAdvance += (charac.advance >> 6);
		}
	}

	void LineEdit2::setText(const std::string& t) {
		mText = t;
	}

}


namespace dal {

	bool TextStream::append(const char* const str) {
		return this->append(str, std::strlen(str));
	}

	bool TextStream::append(const std::string& str) {
		return this->append(str.c_str(), str.size());
	}

	const char* TextStream::get(void) {
		if (0 == this->m_topIndex) return nullptr;

		this->m_buffer.at(this->m_topIndex) = '\0';
		return this->m_buffer.data();
	}

	void TextStream::clear(void) {
		this->m_topIndex = 0;
	}

	unsigned int TextStream::getSize(void) const {
		return this->m_topIndex;
	}

	unsigned int TextStream::getReserved(void) const {
		return this->m_buffer.size();
	}

	// Private

	bool TextStream::append(const char* const ptr, const size_t size) {
		const auto remaining = this->getReserved() - this->getSize();

		if (size > remaining) {
			LoggerGod::getinst().putError("TextStream is full.");
			return false;
		}

		std::memcpy(&this->m_buffer[this->m_topIndex], ptr, size);
		this->m_topIndex += size;
		return true;
	}

}


namespace dal {

	TextBox::TextBox(const CharMaskMapCache& asciiCache)
	:	m_asciiCache(asciiCache)
	{
		constexpr float c = 30.0f / 255.0f;
		this->m_quadRender.setColor(c, c, c, 1.0f);

		this->m_text.append("Sungmin Woo\nwoos8899@gmail.com\n\n");
	}

	TextStream* TextBox::setStrBuf(TextStream* const strBuf) {
		auto tmp = this->m_strBuffer;
		this->m_strBuffer = strBuf;
		return tmp;
	}

	void TextBox::renderOverlay(const UnilocOverlay& uniloc) {
		this->fetchStream();
		m_quadRender.renderQuad(uniloc, this->getDeviceSpace());

		{
			auto info = this->makeScreenSpace();

			const float xInit = info.p1.x + 5.0f;
			float xAdvance = xInit;
			float yHeight = info.p1.y + 20.0f - this->m_scroll;

			for (auto c : this->m_text) {
				if ('\n' == c) {
					yHeight += 20.0f;
					xAdvance = xInit;
					continue;
				}
				else if ('\t' == c) {
					xAdvance += 20.0f;
					continue;
				}

				auto& charac = this->m_asciiCache.at((unsigned int)c);

				// Returns if line is full.
				if (xAdvance + charac.size.x >= info.p2.x) {
					yHeight += 20.0f;
					xAdvance = xInit;
				}

				QuadInfo charQuad;
				
				charQuad.p1.x = xAdvance + charac.bearing.x;
				charQuad.p1.y = yHeight - charac.bearing.y;
				if (charQuad.p1.y < info.p1.y) continue;

				charQuad.p2.x = charQuad.p1.x + charac.size.x;
				charQuad.p2.y = charQuad.p1.y + charac.size.y;
				if (charQuad.p2.y > info.p2.y) return;

				g_charDrawer.setMaskMap(charac.tex);
				g_charDrawer.renderQuad(uniloc, charQuad.screen2device());

				xAdvance += (charac.advance >> 6);
			}
		}
	}

	int TextBox::addScroll(int v) {
		this->m_scroll += v;
		return this->m_scroll;
	}

	void TextBox::onClick(const float x, const float y) {
		constexpr int scrollSpeed = 20;
		const auto halfHeightScreen = this->getPosY() + this->getHeight() * 0.5f;

		if (halfHeightScreen < y) {
			this->m_scroll += scrollSpeed;
		}
		else {
			this->m_scroll -= scrollSpeed;
		}
	}

	// Private

	void TextBox::fetchStream(void) {
		if (nullptr == this->m_strBuffer) return;
		
		auto str = this->m_strBuffer->get();
		if (nullptr == str) return;

		this->m_text += str;
		this->m_strBuffer->clear();
	}

}