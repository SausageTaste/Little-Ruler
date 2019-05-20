#include "o_widget_base.h"

#include <utility>  // for std::swap

#include "p_dalopengl.h"
#include "m_collision2d.h"
#include "s_configs.h"
#include "s_logger_god.h"


namespace {

	auto& g_logger = dal::LoggerGod::getinst();


	class RealQuadRenderer {

	private:
		GLuint mBufferObj;
		GLuint mVao;

		RealQuadRenderer(void) {
			glGenVertexArrays(1, &mVao);
			if (mVao <= 0) throw - 1;
			glGenBuffers(1, &mBufferObj);
			if (mBufferObj <= 0) throw - 1;

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
		static RealQuadRenderer& getinst(void) {
			static RealQuadRenderer inst;
			return inst;
		}

		void renderOverlay(void) {
			glBindVertexArray(this->mVao);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

	};

	glm::vec2 screen2device(const glm::vec2& p, const float winWidth, const float winHeight) {
		return {
			 2.0f * p.x / winWidth - 1.0f,
			-2.0f * p.y / winHeight + 1.0f
		};
	}

	glm::vec2 device2screen(const glm::vec2& p, const float winWidth, const float winHeight) {
		return {
			(p.x + 1.0f) * winWidth / 2.0f,
			(1.0f - p.y) * winHeight / 2.0f
		};
	}

}


namespace dal {

	QuadInfo QuadInfo::screen2device(void) const {
		const auto winWidth = ConfigsGod::getinst().getWinWidth();
		const auto winHeight = ConfigsGod::getinst().getWinHeight();

		QuadInfo newinfo;

		newinfo.p1 = ::screen2device(this->p1, winWidth, winHeight);
		newinfo.p2 = ::screen2device(this->p2, winWidth, winHeight);

		/* Validate PointDev order */ {
			if (newinfo.p1.x > newinfo.p2.x) {
				std::swap(newinfo.p1.x, newinfo.p2.x);
			}
			if (newinfo.p1.y > newinfo.p2.y) {
				std::swap(newinfo.p1.y, newinfo.p2.y);
			}
		}

		return newinfo;
	}

}


namespace dal {

	bool ScreenQuad::isInside(const float x, const float y) {
		const auto quad = this->makeScreenSpace();

		AABB_2D box;
		box.setPoints(quad.p1, quad.p2);
		return box.isInside({ x, y });
	}

	void ScreenQuad::setPosX(const float v) {
		this->m_xPos = v;

		this->makeDeviceSpace();
	}

	void ScreenQuad::setPosY(const float v) {
		this->m_yPos = v;

		this->makeDeviceSpace();
	}

	void ScreenQuad::setWidth(const float v) {
		this->m_width = v;

		this->makeDeviceSpace();
	}

	void ScreenQuad::setHeight(const float v) {
		this->m_height = v;

		this->makeDeviceSpace();
	}

	void ScreenQuad::setAlignMode(const AlignMode mode) {
		this->m_alignMode = mode;
	}

	QuadInfo ScreenQuad::makeScreenSpace(void) const {
		QuadInfo info;

		QuadInfo parInfo;
		if (nullptr != this->m_parent) {
			parInfo = this->m_parent->makeScreenSpace();
		}
		else {
			const auto winWidth = ConfigsGod::getinst().getWinWidth();
			const auto winHeight = ConfigsGod::getinst().getWinHeight();
			parInfo.p1 = { 0.0, 0.0 };
			parInfo.p2 = { winWidth, winHeight };
		}

		switch (this->m_alignMode)
		{

		case AlignMode::upper_left:
			info.p1 = {
				parInfo.p1.x + this->m_xPos,
				parInfo.p1.y + this->m_yPos
			};
			info.p2 = {
				parInfo.p1.x + this->m_xPos + this->m_width,
				parInfo.p1.y + this->m_yPos + this->m_height
			};
			break;
		case AlignMode::upper_right:
			info.p1 = {
				parInfo.p2.x - this->m_xPos - this->m_width,
				parInfo.p1.y + this->m_yPos
			};
			info.p2 = {
				parInfo.p2.x - this->m_xPos,
				parInfo.p1.y + this->m_yPos + this->m_height
			};
			break;

		}

		{
			if (info.p1.x > info.p2.x) {
				g_logger.putWarn("Swap in ScreenQuad::makeScreenSpace");
				std::swap(info.p1.x, info.p2.x);
			}
			if (info.p1.y > info.p2.y) {
				g_logger.putWarn("Swap in ScreenQuad::makeScreenSpace");
				std::swap(info.p1.y, info.p2.y);
			}
		}

		return info;
	}

	const QuadInfo& ScreenQuad::getDeviceSpace(void) const {
		return this->m_deviceSpace;
	}

	void ScreenQuad::onResize(const unsigned int width, const unsigned int height) {
		this->makeDeviceSpace();
	}

	// Private

	void ScreenQuad::makeDeviceSpace(void) {
		this->m_deviceSpace = this->makeScreenSpace().screen2device();
	}

}


namespace dal {

	void QuadRenderer::setColor(const float r, const float g, const float b, const float a) {
		this->m_color.r = r;
		this->m_color.g = g;
		this->m_color.b = b;
		this->m_color.a = a;
	}

	void QuadRenderer::renderQuad(const UnilocOverlay& uniloc, const QuadInfo& devSpc) {
		glUniform2f(uniloc.uPoint1, devSpc.p1.x, devSpc.p1.y);
		glUniform2f(uniloc.uPoint2, devSpc.p2.x, devSpc.p2.y);

		glUniform4f(uniloc.uColor, this->m_color.r, this->m_color.g, this->m_color.b, this->m_color.a);

		this->m_diffuseMap.sendUniform(uniloc.mDiffuseMap, uniloc.mHasDiffuseMap, 0);

		this->m_maskMap.sendUniform(uniloc.mMaskMap, uniloc.mHasMaskMap, 1);
		glUniform1i(uniloc.mUpsideDown_maskMap, 1);

		RealQuadRenderer::getinst().renderOverlay();
	}

}


namespace dal {

	Widget::Widget(Widget* parent)
	: m_parent(parent)
	{
		this->setParent(parent);
	}

}