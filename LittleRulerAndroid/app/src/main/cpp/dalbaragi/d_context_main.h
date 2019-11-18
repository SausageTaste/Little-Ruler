#pragma once

#include "d_context.h"
#include "p_render_master.h"
#include "c_input_apply.h"


namespace dal {

    class InGameCxt : public IContext {

    private:
        RenderMaster& m_renMas;
        SceneGraph& m_scene;
        OverlayMaster& m_overlay;

        InputApplier::PlayerControlWidget m_crtlWidget;

    public:
        InGameCxt(RenderMaster& renMas, SceneGraph& scene, OverlayMaster& overlay, const unsigned width, const unsigned height);

        virtual IContext* update(const float deltaTime) override;
        virtual void onWinResize(const unsigned width, const unsigned height) override;

    };

}