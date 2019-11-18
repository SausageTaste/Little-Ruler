#pragma once

#include "d_context.h"
#include "p_render_master.h"
#include "c_input_apply.h"


namespace dal {

    class InGameCxt : public IContext {

    private:
        ShaderMaster& m_shaders;
        RenderMaster& m_renMas;
        SceneGraph& m_scene;

        PlayerControlWidget m_crtlWidget;

        unsigned m_winWidth, m_winHeight;

    public:
        InGameCxt(ShaderMaster& shaders, RenderMaster& renMas, SceneGraph& scene, const unsigned width, const unsigned height);
        virtual ~InGameCxt(void) override;

        virtual IContext* update(const float deltaTime) override;
        virtual void onWinResize(const unsigned width, const unsigned height) override;

    };

}
