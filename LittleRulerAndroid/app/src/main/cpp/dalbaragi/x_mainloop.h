#pragma once

#include "p_render_master.h"
#include "s_event.h"
#include "c_input_apply.h"
#include "u_timer.h"
#include "x_persist.h"
#include "o_overlay_master.h"


namespace dal {

	class Mainloop : iEventHandler {

		//// Vars ////

	private:
		Timer initTimer;

		bool mFlagQuit;

		Timer mTimer;
		Timer mTimerForFPSReport;

		RenderMaster mRenderMan;
		InputApplier mInputApply;

		//// Funcs ////

	public:
		// Please call this before constructor.
		static void giveScreenResFirst(unsigned int w, unsigned int h);
		static bool isScreenResGiven(void);
		// Windows doesn't need this but Android sure does. Please give it AAssetManager.
		static void giveWhatFilesystemWants(void* androidAssetManager);
		static bool isWhatFilesystemWantsGiven(void);

		Mainloop(PersistState* savedState);
		virtual ~Mainloop(void) override;
		int update(void);
		void onResize(int width, int height);

		PersistState* getSavedState(void);

		virtual void onEvent(const EventStatic& e) override;

	};

}