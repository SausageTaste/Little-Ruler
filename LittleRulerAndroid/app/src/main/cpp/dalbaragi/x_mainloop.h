#pragma once

#include "p_render_master.h"
#include "s_event.h"
#include "c_input_apply.h"
#include "u_timer.h"
#include "o_overlay_master.h"
#include "p_resource.h"
#include "g_actor.h"


namespace dal {

    class Mainloop : iEventHandler {

        //// Vars ////

    private:
        bool m_flagQuit;

        Timer m_timer;
        Timer m_timerForFPSReport;

        StrangeEulerCamera m_camera;
        Player m_player;

        ShaderMaster m_shader;
        ResourceMaster m_resMas;
        SceneMaster m_scene;
        OverlayMaster m_overlayMas;
        RenderMaster m_renderMan;
        InputApplier m_inputApply;

        //// Funcs ////

    public:
        // Windows doesn't need this but Android sure does. Please give it AAssetManager.
        static void giveWhatFilesystemWants(void* androidAssetManager, const char* const sdcardPath);
        static bool isWhatFilesystemWantsGiven(void);

        Mainloop(const unsigned int winWidth, const unsigned int winHeight);
        virtual ~Mainloop(void) override;
        int update(void);
        void onResize(int width, int height);

        virtual void onEvent(const EventStatic& e) override;

    };

}