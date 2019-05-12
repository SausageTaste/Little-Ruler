#pragma once

#include <array>

#include "o_widget_primitive.h"
#include "p_shader_master.h"
#include "p_uniloc.h"
#include "s_event.h"
#include "o_widget_textbox.h"
#include "p_globalfsm.h"
#include "o_text_cache.h"
#include "p_resource.h"


namespace dal {

	class OverlayMaster : public iEventHandler {

		//////// Vars ////////

	private:
		ResourceMaster& m_resMas;
		ShaderMaster& m_shaderMas;

		CharMaskMapCache m_asciiCache;

		GlobalFSM mGlobalFSM;

		

		TextBox mDisplayFPS;
		TextBox mLineEdit;

	public:
		std::array<QuadPrimitive, 11> mBoxesForTouchPoint;
		
		//////// Funcs ////////

		OverlayMaster(ResourceMaster& resMas, ShaderMaster& shaderMas);
		virtual ~OverlayMaster(void) override;
		virtual void onEvent(const EventStatic& e) override;
		
		void render(void);

		void setDisplayedFPS(unsigned int fps);

	};

}