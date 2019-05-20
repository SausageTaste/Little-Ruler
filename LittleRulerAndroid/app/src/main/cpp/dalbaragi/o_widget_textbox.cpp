#include "o_widget_textbox.h"

#include <cstring>
#include <string>

#include <fmt/format.h>

#include "s_logger_god.h"
#include "m_collision2d.h"
#include "s_scripting.h"
#include "p_globalfsm.h"
#include "s_configs.h"


using namespace std::string_literals;


namespace {

	auto g_logger = dal::LoggerGod::getinst();

	dal::QuadRenderer g_charDrawer;

	constexpr float g_darkTheme = 30.0f / 255.0f;


	void toggleGameState(void) {
		dal::EventStatic e;
		e.type = dal::EventType::global_fsm_change;

		const auto curState = dal::ConfigsGod::getinst().getGlobalGameState();

		switch (curState) {
		case dal::GlobalGameState::game:
			e.intArg1 = static_cast<int>(dal::GlobalGameState::menu); break;
		case dal::GlobalGameState::menu:
			e.intArg1 = static_cast<int>(dal::GlobalGameState::game); break;
		}

		dal::EventGod::getinst().notifyAll(e);
	}

}


namespace {

	// from https://github.com/s22h/cutils/blob/master/include/s22h/unicode.h

	constexpr uint32_t UTF8_ONE_BYTE_MASK = 0x80;
	constexpr uint32_t UTF8_ONE_BYTE_BITS = 0;
	constexpr uint32_t UTF8_TWO_BYTES_MASK = 0xE0;
	constexpr uint32_t UTF8_TWO_BYTES_BITS = 0xC0;
	constexpr uint32_t UTF8_THREE_BYTES_MASK = 0xF0;
	constexpr uint32_t UTF8_THREE_BYTES_BITS = 0xE0;
	constexpr uint32_t UTF8_FOUR_BYTES_MASK = 0xF8;
	constexpr uint32_t UTF8_FOUR_BYTES_BITS = 0xF0;
	constexpr uint32_t UTF8_CONTINUATION_MASK = 0xC0;
	constexpr uint32_t UTF8_CONTINUATION_BITS = 0x80;

	size_t utf8_codepoint_size(const uint8_t byte) {
		if ((byte & UTF8_ONE_BYTE_MASK) == UTF8_ONE_BYTE_BITS) {
			return 1;
		}

		if ((byte & UTF8_TWO_BYTES_MASK) == UTF8_TWO_BYTES_BITS) {
			return 2;
		}

		if ((byte & UTF8_THREE_BYTES_MASK) == UTF8_THREE_BYTES_BITS) {
			return 3;
		}

		if ((byte & UTF8_FOUR_BYTES_MASK) == UTF8_FOUR_BYTES_BITS) {
			return 4;
		}

		return 0;
	}

	uint32_t convert_utf8_to_utf32(const uint8_t* begin, const uint8_t* end) {
		const auto cp_size = end - begin;
		assert((utf8_codepoint_size(*begin)) == cp_size);
		assert(1 <= cp_size && cp_size <= 4);

		uint32_t buffer = 0;

		switch (cp_size) {

		case 1:
			buffer = ((uint32_t)begin[0] & ~UTF8_ONE_BYTE_MASK);
			return buffer;
		case 2:
			buffer =
				((uint32_t)begin[0] & ~UTF8_TWO_BYTES_MASK) << 6 |
				((uint32_t)begin[1] & ~UTF8_CONTINUATION_MASK);
			return buffer;
		case 3:
			buffer =
				((uint32_t)begin[0] & ~UTF8_THREE_BYTES_MASK) << 12 |
				((uint32_t)begin[1] & ~UTF8_CONTINUATION_MASK) << 6 |
				((uint32_t)begin[2] & ~UTF8_CONTINUATION_MASK);
			return buffer;
		case 4:
			buffer =
				((uint32_t)begin[0] & ~UTF8_FOUR_BYTES_MASK) << 18 |
				((uint32_t)begin[1] & ~UTF8_CONTINUATION_MASK) << 12 |
				((uint32_t)begin[2] & ~UTF8_CONTINUATION_MASK) << 6 |
				((uint32_t)begin[3] & ~UTF8_CONTINUATION_MASK);
			return buffer;
		default:
			// this should never happen, since we check validity of the string
			fprintf(stderr, "utf8_to_utf32: invalid byte in UTF8 string.\n");
			return 0;

		}
	}

}


// Label
namespace dal {

	Label::Label(Widget* parent, UnicodeCache& asciiCache)
	:	Widget(parent),
		m_textColor(1.0, 1.0, 1.0, 1.0),
		m_unicodeCache(asciiCache)
	{
		this->setPosX(10.0f);
		this->setPosY(40.0f);
		this->setWidth(400.0f);
		this->setHeight(20.0f);

		this->m_background.setColor(g_darkTheme, g_darkTheme, g_darkTheme, 1.0f);
	}

	void Label::onClick(const float x, const float y) {
		toggleGameState();
	}

	void Label::renderOverlay(const UnilocOverlay& uniloc) {
		this->m_background.renderQuad(uniloc, this->getDeviceSpace());

		const auto screenInfo = this->makeScreenSpace();

		float xAdvance = screenInfo.p1.x;
		const float boxHeight = screenInfo.p2.y - screenInfo.p1.y;
		const float yHeight = screenInfo.p2.y - boxHeight / 4.0f;


		auto header = this->m_text.begin();
		const auto end = this->m_text.end();

		while (end != header) {
			uint32_t c = 0;
			{
				const auto ch = *header;
				const auto codeSize = utf8_codepoint_size(*header);
				if (codeSize > 1) {
					uint8_t buf[4];
					for (size_t i = 0; i < codeSize; i++) {
						buf[i] = *header++;
					}

					c = convert_utf8_to_utf32(buf, buf + codeSize);
				}
				else {
					c = static_cast<uint32_t>(*header++);
				}
			}

			auto& charac = this->m_unicodeCache.at(c);

			QuadInfo charQuad;

			charQuad.p1.x = xAdvance + charac.bearing.x;
			charQuad.p1.y = yHeight - charac.bearing.y;

			charQuad.p2.x = charQuad.p1.x + charac.size.x;
			charQuad.p2.y = charQuad.p1.y + charac.size.y;

			QuadRenderer::statelessRender(uniloc, charQuad.screen2device(), this->m_textColor, &charac.tex);

			xAdvance += (charac.advance >> 6);
		}
	}

	void Label::setText(const std::string& t) {
		this->m_text = t;
	}

	const std::string& Label::getText(void) const {
		return this->m_text;
	}

	void Label::setTextColor(const float r, const float g, const float b, const float a) {
		this->m_textColor = { r, g, b, a };
	}

	void Label::setBackgroundColor(const float r, const float g, const float b, const float a) {
		this->m_background.setColor(r, g, b, a);
	}

}


namespace dal {

	LineEdit::LineEdit(Widget* parent, UnicodeCache& asciiCache)
	:	Label(parent, asciiCache)
	{

	}

	void LineEdit::onClick(const float x, const float y) {

	}

	void LineEdit::onKeyInput(const char* const str) {
		auto text = this->getText();
		const auto len = std::strlen(str);

		for (size_t i = 0; i < len; i++) {
			const auto c = str[i];

			switch (c) {

			case '\n':
				this->onReturn();
				break;
			case '\b':
				if (text.empty()) break;
				text.pop_back();
				break;
			case '\t':
				text.append("    ");  // 4 whitespaces. 
				break;
			case '\0':
				return;
			default:
				text += c;
				break;

			}
		}

		this->setText(text);
	}

	void LineEdit::onReturn(void) {
		Lua::getinst().doString(this->getText().c_str());
		this->setText("");
	}

	void LineEdit::onFocusChange(bool isFocus) {
		if (isFocus) {
			this->setTextColor(1.0f, 1.0f, 1.0f, 1.0f);
		}
		else {
			this->setTextColor(0.6f, 0.6f, 0.6f, 1.0f);
		}
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
			return false;
		}

		std::memcpy(&this->m_buffer[this->m_topIndex], ptr, size);
		this->m_topIndex += size;
		return true;
	}

}


namespace dal {

	TextBox::TextBox(Widget* parent, UnicodeCache& unicodes)
	:	Widget(parent),
		m_unicodes(unicodes)
	{
		this->m_quadRender.setColor(g_darkTheme, g_darkTheme, g_darkTheme, 1.0f);
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

			auto header = this->m_text.begin();
			const auto end = this->m_text.end();

			while (end != header) {
				uint32_t c = 0;
				{
					const auto ch = *header;
					const auto codeSize = utf8_codepoint_size(*header);
					if (codeSize > 1) {
						std::vector<uint8_t> buffer;
						for (size_t i = 0; i < codeSize; i++) {
							buffer.push_back(*header++);
						}

						c = convert_utf8_to_utf32(&buffer[0], &buffer[0] + buffer.size());
					}
					else {
						c = static_cast<uint32_t>(*header++);
					}
				}
				
				if ('\n' == c) {
					yHeight += 20.0f;
					xAdvance = xInit;
					continue;
				}
				else if ('\t' == c) {
					xAdvance += 20.0f;
					continue;
				}

				auto& charac = this->m_unicodes.at(c);

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