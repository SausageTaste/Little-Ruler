#include "o_widget_textbox.h"

#include <cstring>
#include <string>

#include "s_logger_god.h"
#include "m_collision2d.h"
#include "s_scripting.h"


using namespace std::string_literals;


namespace dal {

	LineEdit::LineEdit(void) {
		mCharDrawer.setColor(1.0, 1.0, 1.0);
		mMainBox.setColor(0.0, 0.0, 0.0);
	}

	void LineEdit::onKeyInput(const char c) {
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

	void LineEdit::onReturn(void) {
		Lua::getinst().doString(mText.c_str());
		mText.clear();
	}

	void LineEdit::renderOverlay(const CharMaskMapCache& asciiCache, const UnilocOverlay& uniloc) {
		mMainBox.renderOverlay(uniloc);

		auto& p1 = mMainBox.getPointScr1();
		auto& p2 = mMainBox.getPointScr2();

		float xAdvance = p1.x;
		const float boxHeight = p2.y - p1.y;
		const float yHeight = p2.y - boxHeight / 4.0f;

		for (auto c : mText) {
			auto& charac = asciiCache.at((unsigned int)c);

			const float xPos = xAdvance + charac.bearing.x;
			const float yPos = yHeight - charac.bearing.y;

			const float xPos2 = xPos + charac.size.x;
			const float yPos2 = yPos + charac.size.y;

			mCharDrawer.setMaskMap(charac.tex);
			mCharDrawer.setPointScrs(xPos, yPos, xPos2, yPos2);
			mCharDrawer.renderOverlay(uniloc);

			xAdvance += (charac.advance >> 6);
		}

		/*
		mMainBox.renderOverlay(uniloc);
		const float textPortion = 0.8f;
		
		float x, y;
		mMainBox.getPointScr1(&x, &y);

		float largestHeight = 0.0f;
		for (auto c : mText) {
			const auto height = float(asciiCache.at((unsigned int)c).size.y);
			if (height > largestHeight) {
				largestHeight = float(height);
			}
		}

		//mScale = textPortion * mMainBox.getHeight() / largestHeight;
		mScale = 1.0f;

		for (auto c : mText) {
			auto& charac = asciiCache.at((unsigned int)c);

			const float xPos = x + charac.bearing.x * mScale;
			const float verticalReplaceAsScale = ((1.0f - textPortion) / 2.0f * mMainBox.getHeight());
			const float yPos = y - (2.0f * charac.size.y - charac.bearing.y - largestHeight) * mScale + verticalReplaceAsScale;

			const float xPos2 = xPos + charac.size.x * mScale;
			const float yPos2 = yPos + charac.size.y * mScale;
			
			const float xAxisOverflow = xPos2 - mMainBox.getPointScr2X();
			if (xAxisOverflow > 0.0f) {
				//xPos2 -= xAxisOverflow;
				continue;
			}
			//if (xPos > xPos2) continue;
			
			mCharDrawer.setMaskMap(charac.tex);
			mCharDrawer.setPointScrs(xPos, yPos, xPos2, yPos2);
			mCharDrawer.renderOverlay(uniloc);

			x += (charac.advance >> 6) * mScale;
		}
		*/
	}

	void LineEdit::onResize(void) {
		mMainBox.convertScrIntoDev();
	}

	void LineEdit::setPos(float x, float y) {
		this->mMainBox.moveCornerTo_screenCoord(x, y);
	}

	void LineEdit::setSize(float w, float h) {
		this->mMainBox.setWidth(w);
		this->mMainBox.setHeight(h);
	}

	void LineEdit::setText(const char* const t) {
		mText = t;
	}

	void LineEdit::setTextColor(const float r, const float g, const float b) {
		this->mCharDrawer.setColor(r, g, b);
	}

	bool LineEdit::isInside(const glm::vec2& p) const {
		const auto p1 = mMainBox.getPointScr1();
		const auto& p2 = mMainBox.getPointScr2();

		AABB_2D box;
		box.setPoints(p1, p2);
		return box.isInside(p);
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
			static QuadPrimitive charDrawer;

			auto info = this->getScreenSpace();

			const float xInit = info.p1.x + 5.0f;
			float xAdvance = xInit;
			float yHeight = info.p1.y + 20.0f;

			for (auto c : this->m_text) {
				if ('\n' == c) {
					yHeight += 20.0f;
					xAdvance = xInit;
					continue;
				}
				else if ('\t' == c) {
					xAdvance += 10.0f;
					continue;
				}

				auto& charac = this->m_asciiCache.at((unsigned int)c);
				
				const float xPos = xAdvance + charac.bearing.x;
				const float yPos = yHeight - charac.bearing.y;

				const float xPos2 = xPos + charac.size.x;
				const float yPos2 = yPos + charac.size.y;

				charDrawer.setMaskMap(charac.tex);
				charDrawer.setPointScrs(xPos, yPos, xPos2, yPos2);
				charDrawer.renderOverlay(uniloc);

				xAdvance += (charac.advance >> 6);
			}
		}

		
	}

	int TextBox::addScroll(int v) {
		this->m_scroll += v;
		return this->m_scroll;
	}

	// Private

	void TextBox::fetchStream(void) {
		if (nullptr == this->m_strBuffer) return;
		
		auto str = this->m_strBuffer->get();
		if (nullptr == str) return;

		this->m_text += str;
		this->m_strBuffer->clear();
		LoggerGod::getinst().putInfo("Fetched: "s + str);
	}

}