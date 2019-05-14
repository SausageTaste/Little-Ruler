#pragma once

#include <array>

#include "s_event.h"
#include "p_resource.h"
#include "p_globalfsm.h"
#include "p_shader_master.h"
#include "o_text_cache.h"
#include "o_widget_textbox.h"
#include "o_widget_base.h"


namespace dal {

	class OverlayMaster : public iEventHandler {

		//////// Vars ////////

	private:
		ResourceMaster& m_resMas;
		const ShaderMaster& m_shaderMas;

		CharMaskMapCache m_asciiCache;

		GlobalFSM mGlobalFSM;

		LineEdit mDisplayFPS;
		LineEdit mLineEdit;

		TextStream m_strBuffer;
		TextBox m_texBox;

		IKeyInputTaker* m_keyTaker;

	public:
		std::array<QuadPrimitive, 11> mBoxesForTouchPoint;
		
		//////// Funcs ////////

		OverlayMaster(ResourceMaster& resMas, const ShaderMaster& shaderMas);
		virtual ~OverlayMaster(void) override;
		virtual void onEvent(const EventStatic& e) override;

		void onClick(const float x, const float y);
		void onKeyInput(const std::string& c);
		
		void render(void);

		void setDisplayedFPS(unsigned int fps);

	};

}