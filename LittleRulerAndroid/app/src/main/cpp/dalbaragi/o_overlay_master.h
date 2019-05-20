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

	private:
		class TextStreamChannel : public dal::ILoggingChannel {

		private:
			dal::TextStream& m_texStream;

		public:
			TextStreamChannel(dal::TextStream& texStream);

			virtual void verbose(const char* const str) override;
			virtual void debug(const char* const str) override;
			virtual void info(const char* const str) override;
			virtual void warn(const char* const str) override;
			virtual void error(const char* const str) override;
			virtual void fatal(const char* const str) override;

		};

		//////// Vars ////////

	private:
		ResourceMaster& m_resMas;
		const ShaderMaster& m_shaderMas;

		UnicodeCache m_unicodes;

		GlobalGameState mGlobalFSM;

		Label* mDisplayFPS;
		TextStream m_strBuffer;
		TextStreamChannel m_texStreamCh;

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