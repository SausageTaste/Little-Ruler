#pragma once

#include <array>

#include "o_widget_primitive.h"
#include "p_shader.h"
#include "p_uniloc.h"
#include "s_event.h"
#include "o_widget_textbox.h"
#include "p_globalfsm.h"
#include "p_texture.h"
#include "o_text_cache.h"
#include "p_resource.h"


namespace dal {

	class OverlayMaster : public iEventHandler {

		//////// Vars ////////

	private:
		ResourceMaster& m_resMas;
		CharMaskMapCache m_asciiCache;

		GlobalFSM mGlobalFSM;

		ShaderProgram mShaderOverlay;
		UnilocOverlay mUnilocOverlay;

		TextBox mDisplayFPS;
		TextBox mLineEdit;

	public:
		std::array<QuadPrimitive, 11> mBoxesForTouchPoint;
		QuadPrimitive mDebugPlane;
		
		//////// Funcs ////////

		OverlayMaster(TextureMaster& texMaster, ResourceMaster& resMas);
		virtual ~OverlayMaster(void) override;
		virtual void onEvent(const EventStatic& e) override;
		
		void render(void);

		void setDisplayedFPS(unsigned int fps);

	};

}