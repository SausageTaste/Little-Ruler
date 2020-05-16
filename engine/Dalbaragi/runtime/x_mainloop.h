#pragma once

#include <d_glyph.h>

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
        TaskMaster m_task;
        ShaderMaster m_shader;
        ResourceMaster m_resMas;
        PhysicsWorld m_phyworld;
        SceneGraph m_scene;
        RenderMaster m_renderMan;
        GlyphMaster m_glyph;
        Config m_config;

        // Contexts
        std::vector<std::unique_ptr<IContext>> m_contexts;
        IContext* m_currentContext;

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
