#pragma once

#include <vector>
#include <memory>

#include "d_context.h"


namespace dal {

    class ShaderMaster;
    class RenderMaster;
    class SceneGraph;
    class TaskMaster;


    std::vector<std::unique_ptr<IContext>> initContexts(
        const unsigned width, const unsigned height,
        ShaderMaster& shaders, RenderMaster& renMas, SceneGraph& scene, TaskMaster& taskMas
    );

}
