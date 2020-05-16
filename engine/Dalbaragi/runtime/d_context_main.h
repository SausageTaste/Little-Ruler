#pragma once

#include <vector>
#include <memory>

#include "d_context.h"


namespace dal {

    class ShaderMaster;
    class RenderMaster;
    class SceneGraph;
    class TaskMaster;
    class PhysicsWorld;
    class GlyphMaster;


    struct Managers {
        ShaderMaster&  m_shaders;
        RenderMaster&  m_renMas;
        SceneGraph&    m_scene;
        TaskMaster&    m_taskMas;
        PhysicsWorld&  m_phyworld;
        GlyphMaster&   m_glyph;

        Managers(
            ShaderMaster& shaders,
            RenderMaster& renMas,
            SceneGraph& scene,
            TaskMaster& taskMas,
            PhysicsWorld& phyworld,
            GlyphMaster& glyph
        )
            : m_shaders(shaders)
            , m_renMas(renMas)
            , m_scene(scene)
            , m_taskMas(taskMas)
            , m_phyworld(phyworld)
            , m_glyph(glyph)
        {

        }
    };

    std::vector<std::unique_ptr<IContext>> initContexts(const unsigned width, const unsigned height, Managers& managers);

}
