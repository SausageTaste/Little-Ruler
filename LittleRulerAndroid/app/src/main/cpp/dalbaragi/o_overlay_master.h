#pragma once

#include <array>
#include <list>

#include "s_event.h"
#include "p_resource.h"
#include "p_globalfsm.h"
#include "p_shader_master.h"
#include "o_text_cache.h"
#include "o_widget_textbox.h"
#include "o_widget_base.h"
#include "s_logger_god.h"


namespace dal {

	class OverlayMaster : public iEventHandler {

		//////// Vars ////////

	private:
		ResourceMaster& m_resMas;
		const ShaderMaster& m_shaderMas;

		AsciiCache m_asciiCache;
		UnicodeCache m_unicodes;

		GlobalFSM mGlobalFSM;

		LineEdit* mDisplayFPS;
		TextStream m_strBuffer;

		std::list<Widget*> m_widgets;

	public:
		std::array<QuadPrimitive, 11> mBoxesForTouchPoint;
		
		//////// Funcs ////////

		OverlayMaster(ResourceMaster& resMas, const ShaderMaster& shaderMas);
		virtual ~OverlayMaster(void) override;
		virtual void onEvent(const EventStatic& e) override;

		void onClick(const float x, const float y);
		void onDrag(const glm::vec2& start, const glm::vec2& end);
		void onKeyInput(const std::string& c);
		
		void render(void) const;

		void setDisplayedFPS(const unsigned int fps);

	};

}