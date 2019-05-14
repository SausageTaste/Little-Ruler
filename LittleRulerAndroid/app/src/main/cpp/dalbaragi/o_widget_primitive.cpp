#include "o_widget_primitive.h"

#include <string>

#include "s_configs.h"
#include "s_logger_god.h"


using namespace std::string_literals;


namespace {

	class QuadRenderer {

	private:
		GLuint mBufferObj;
		GLuint mVao;

		QuadRenderer(void) {
			glGenVertexArrays(1, &mVao);
			if (mVao <= 0) throw -1;
			glGenBuffers(1, &mBufferObj);
			if (mBufferObj <= 0) throw -1;

			glBindVertexArray(mVao);

			/* Vertices */ {
				GLfloat vertices[12] = {
						0, 1,
						0, 0,
						1, 0,
						0, 1,
						1, 0,
						1, 1
				};
				auto size = 12 * sizeof(float);

				glBindBuffer(GL_ARRAY_BUFFER, this->mBufferObj);
				glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
			}

			glBindVertexArray(0);
		}
	public:
		static QuadRenderer& getinst(void) {
			static QuadRenderer inst;
			return inst;
		}

		void renderOverlay(void) {
			glBindVertexArray(this->mVao);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		
	};

	glm::vec2 scrCoord2devCoord(const glm::vec2& p, const float winWidth, const float winHeight) {
		return {
			 2.0f * p.x / winWidth  - 1.0f,
			-2.0f * p.y / winHeight + 1.0f
		};
	}

	glm::vec2 devCoord2scrCoord(const glm::vec2& p, const float winWidth, const float winHeight) {
		return {
			(p.x + 1.0f) * winWidth / 2.0f,
			(1.0f - p.y) * winHeight / 2.0f
		};
	}

}


namespace dal {

	QuadPrimitive::QuadPrimitive(void)
	:	mColor(1.0, 1.0, 1.0, 1.0)
	{
		mPointScr1 = { 100, 100 };
		mPointScr2 = { 200, 200 };

		this->convertScrIntoDev();
	}

	void QuadPrimitive::moveCenterTo_screenCoord(float x, float y) {
		glm::vec2 inputVec{ x, y };
		auto halfDeltaVec = (mPointScr2 - mPointScr1) / 2.0f;

		mPointScr1 = inputVec - halfDeltaVec;
		mPointScr2 = inputVec + halfDeltaVec;

		this->convertScrIntoDev();
	}

	void QuadPrimitive::moveCornerTo_screenCoord(float x, float y) {
		glm::vec2 inputVec{ x, y };
		auto deltaVec = (mPointScr2 - mPointScr1) / 2.0f;

		mPointScr1 = inputVec;
		mPointScr2 = inputVec + deltaVec;

		this->convertScrIntoDev();
	}

	void QuadPrimitive::setWidth(const float v) {
		mPointScr2.x = mPointScr1.x + v;
		this->convertScrIntoDev();
	}

	void QuadPrimitive::setHeight(const float v) {
		mPointScr2.y = mPointScr1.y + v;
		this->convertScrIntoDev();
	}

	void QuadPrimitive::setColor(const float r, const float g, const float b) {
		mColor.r = r;
		mColor.g = g;
		mColor.b = b;
	}

	void QuadPrimitive::setColor(glm::vec3 v) {
		mColor.x = v.x;
		mColor.y = v.y;
		mColor.z = v.z;
	}

	void QuadPrimitive::setDiffuseMap(const TextureHandle2& tex) {
		mDiffuseMap = tex;
	}

	void QuadPrimitive::setMaskMap(const TextureHandle2& tex) {
		this->mMaskMap = tex;
	}

	const glm::vec2& QuadPrimitive::getPointScr1(void) const {
		return mPointScr1;
	}

	const glm::vec2& QuadPrimitive::getPointScr2(void) const {
		return mPointScr2;
	}

	float QuadPrimitive::getWidth(void) const {
		return mPointScr2.x - mPointScr1.x;
	}

	float QuadPrimitive::getHeight(void) const {
		return mPointScr2.y - mPointScr1.y;
	}

	void QuadPrimitive::setTransparency(const float a) {
		mColor.a = a;
	}

	void QuadPrimitive::setPointScrs(const float x1, const float y1, const float x2, const float y2) {
		mPointScr1 = { x1, y1 };
		mPointScr2 = { x2, y2 };
		this->convertScrIntoDev();
	}

	void QuadPrimitive::renderOverlay(const UnilocOverlay& uniloc) {
		glUniform2f(uniloc.uPoint1, mPointDev1.x, mPointDev1.y);
		glUniform2f(uniloc.uPoint2, mPointDev2.x, mPointDev2.y);

		glUniform4f(uniloc.uColor, mColor.r, mColor.g, mColor.b, mColor.a);

		mDiffuseMap.sendUniform(uniloc.mDiffuseMap, uniloc.mHasDiffuseMap, 0);
		
		mMaskMap.sendUniform(uniloc.mMaskMap, uniloc.mHasMaskMap, 1);
		glUniform1i(uniloc.mUpsideDown_maskMap, 1);
	
		QuadRenderer::getinst().renderOverlay();
	}

	// Private

	void QuadPrimitive::convertScrIntoDev(void) {
		const float winWidth = (float)ConfigsGod::getinst().getWinWidth();
		const float winHeight = (float)ConfigsGod::getinst().getWinHeight();

		mPointDev1 = scrCoord2devCoord(mPointScr1, winWidth, winHeight);
		mPointDev2 = scrCoord2devCoord(mPointScr2, winWidth, winHeight);

		/* Validate PointDev order */  {
			if (mPointDev1.x > mPointDev2.x) {
				std::swap(mPointDev1.x, mPointDev2.x);
			}
			if (mPointDev1.y > mPointDev2.y) {
				std::swap(mPointDev1.y, mPointDev2.y);
			}
			if (mPointScr1.x > mPointScr2.x) {
				std::swap(mPointScr1.x, mPointScr2.x);
			}
			if (mPointScr1.y > mPointScr2.y) {
				std::swap(mPointScr1.y, mPointScr2.y);
			}
		}
	}

}