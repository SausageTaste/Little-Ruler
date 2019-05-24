#pragma once

#include "o_widget_base.h"
#include "p_dalopengl.h"
#include "p_resource.h"


namespace dal {

	class TextureView : public Widget {

	private:
		QuadRenderer m_quadRender;

	public:
		explicit TextureView(Widget* parent, Texture* const tex = nullptr);
		virtual void renderOverlay(const UnilocOverlay& uniloc) override;
		void setTexture(Texture* const tex);

	};

}