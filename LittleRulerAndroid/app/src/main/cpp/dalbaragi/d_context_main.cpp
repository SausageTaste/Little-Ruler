#include "d_context_main.h"


namespace dal {

    InGameCxt::InGameCxt(RenderMaster& renMas, SceneGraph& scene, OverlayMaster& overlay, const unsigned width, const unsigned height)
        : m_renMas(renMas)
        , m_scene(scene)
        , m_overlay(overlay)
        , m_crtlWidget(static_cast<float>(width), static_cast<float>(height))
    {
        this->m_overlay.giveBackgroudWidgetRef(&this->m_crtlWidget);
    }

    IContext* InGameCxt::update(const float deltaTime) {
        // Process input
        {
            this->m_overlay.updateInputs();

            auto& state = this->m_scene.m_entities.get<cpnt::CharacterState>(this->m_scene.m_player);
            const auto winSize = GlobalStateGod::getinst().getWinSizeFloat();
            const auto info = this->m_crtlWidget.getMoveInfo(deltaTime, winSize.x, winSize.y);
            state.update(deltaTime, info);
        }

        TaskGod::getinst().update();

        this->m_scene.update(deltaTime);
        this->m_renMas.update(deltaTime);
        this->m_renMas.render(this->m_scene.m_entities);
        this->m_overlay.render();

        return this;
    }

    void InGameCxt::onWinResize(const unsigned width, const unsigned height) {
        this->m_crtlWidget.onParentResize(width, height);
    }

}