#pragma once


#include "p_render_master.h"
#include "c_input_apply.h"
#include "u_timer.h"
#include "p_resource.h"
#include "g_actor.h"
#include "d_context_main.h"


namespace dal {

    class Mainloop {

        //// Vars ////

    private:
        // Managers
        ShaderMaster m_shader;
        ResourceMaster m_resMas;
        SceneGraph m_scene;
        RenderMaster m_renderMan;

        // Contexts
        IContext* m_currentContext;
        InGameCxt m_cxtIngame;

        // Misc
        TimerThatCaps m_timer;
        bool m_flagQuit;

        //// Funcs ////

    public:
        // Windows doesn't need this but Android sure does. Please give it AAssetManager.
        static void giveWhatFilesystemWants(void* androidAssetManager, const char* const sdcardPath);
        static bool isWhatFilesystemWantsGiven(void);

        Mainloop(const unsigned int winWidth, const unsigned int winHeight);
        ~Mainloop(void);
        int update(void);
        void onResize(unsigned int width, unsigned int height);

    };

}
