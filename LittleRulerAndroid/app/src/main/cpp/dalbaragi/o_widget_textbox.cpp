#include "o_widget_textbox.h"

#include <cstring>
#include <string>

#include "s_logger_god.h"
#include "m_collision2d.h"
#include "s_scripting.h"


using namespace std::string_literals;


namespace dal {

	LineEdit::LineEdit(void)
	:	mScale(1.0f)
	{
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

	bool StringBuffer::append(const char* const str) {
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

	unsigned int StringBuffer::getSize(void) const {
		return this->m_topIndex;
	}

	unsigned int StringBuffer::getReserved(void) const {
		return this->m_buffer.size();
	}

}


namespace dal {

	StringBuffer* TextBox::setStrBuf(StringBuffer* const strBuf) {
		auto tmp = this->m_strBuffer;
		this->m_strBuffer = strBuf;
		return tmp;
	}

}