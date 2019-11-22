#pragma once

#include <memory>

#include "d_context.h"
#include "p_render_master.h"
#include "c_input_apply.h"


namespace dal {

    std::vector<std::unique_ptr<IContext>> initContexts(const unsigned width, const unsigned height, ShaderMaster& shaders, RenderMaster& renMas, SceneGraph& scene);

}
