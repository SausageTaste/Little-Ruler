#pragma once

#include "d_context.h"
#include "p_render_master.h"
#include "c_input_apply.h"


namespace dal {

    class InGameCxt : public IContext {

    private:
        RenderMaster& m_renMas;
        InputApplier& m_input;
        SceneGraph& m_scene;
        OverlayMaster& m_overlay;

    public:
        InGameCxt(RenderMaster& renMas, InputApplier& input, SceneGraph& scene, OverlayMaster& overlay);

        virtual IContext* update(const float deltaTime) override;

    };

}
