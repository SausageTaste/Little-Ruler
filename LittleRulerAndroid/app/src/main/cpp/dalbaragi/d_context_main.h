#pragma once

#include "d_context.h"
#include "p_render_master.h"


namespace dal {

    class InGameCxt : public IContext {

    private:
        RenderMaster& m_renMas;

    public:
        InGameCxt(RenderMaster& renMas);

        virtual IContext* update(const float deltaTime) override;

    };

}