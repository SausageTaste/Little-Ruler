#include "o_widget_textbox.h"

#include <cstring>


#include "s_logger_god.h"
#include "m_collision2d.h"
#include "s_scripting.h"
#include "p_texture.h"


using namespace std;


namespace dal {

	TextBox::TextBox(void)
	:	mScale(1.0f)
	{
		mCharDrawer.setColor(1.0, 1.0, 1.0);
		mMainBox.setColor(0.0, 0.0, 0.0);
	}

	void TextBox::give(const char* const str) {
		auto len = strlen(str);
		for (unsigned int i = 0; i < len; i++) {
			switch (str[i]) {
			case '\n':
				this->onReturn();
				break;
			case '\b':
				mText.pop_back();
				break;
			case '\t':
				mText += "    "s;
				break;
			case '\0':
				return;
			default:
				mText.push_back(str[i]);
				break;
			}
		}
	}

	void TextBox::onReturn(void) {
		Lua::getinst().doString(mText.c_str());
		mText.clear();
	}

	void TextBox::renderOverlay(const CharMaskMapCache& asciiCache, const UnilocOverlay& uniloc) {
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

		mScale = textPortion * mMainBox.getHeight() / largestHeight;

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
	}

	void TextBox::onResize(void) {
		mMainBox.convertScrIntoDev();
	}

	void TextBox::setPos(float x, float y) {
		this->mMainBox.moveCornerTo_screenCoord(x, y);
	}

	void TextBox::setSize(float w, float h) {
		this->mMainBox.setWidth(w);
		this->mMainBox.setHeight(h);
	}

	void TextBox::setText(const char* const t) {
		mText = t;
	}

	bool TextBox::isInside(const glm::vec2& p) const {
		float x, y;
		mMainBox.getPointScr1(&x, &y);
		const auto p1 = glm::vec2{ x, y };
		const auto& p2 = mMainBox.getPointScr2();

		AABB_2D box;
		box.setPoints(p1, p2);
		return box.isInside(p);
	}

}