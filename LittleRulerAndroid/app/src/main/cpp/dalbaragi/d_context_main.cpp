#include "d_context_main.h"


namespace dal {

    InGameCxt::InGameCxt(RenderMaster& renMas, InputApplier& input, SceneGraph& scene, OverlayMaster& overlay)
        : m_renMas(renMas)
        , m_input(input)
        , m_scene(scene)
        , m_overlay(overlay)
    {

    }

    IContext* InGameCxt::update(const float deltaTime) {
        // Process input
        {
            this->m_overlay.updateInputs();
            this->m_input.apply(deltaTime, this->m_scene.m_playerCam, this->m_scene.m_entities.get<cpnt::CharacterState>(this->m_scene.m_player));
        }

        TaskGod::getinst().update();

        this->m_scene.update(deltaTime);
        this->m_renMas.update(deltaTime);
        this->m_renMas.render(this->m_scene.m_entities);
        this->m_overlay.render();

        return this;
    }

}