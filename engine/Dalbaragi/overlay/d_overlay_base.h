#pragma once

#include <functional>

#include <glm/glm.hpp>


namespace dal {

    class OverlayTexture {

    public:
        virtual ~OverlayTexture(void) = default;

        virtual void init_maskMap(const uint8_t* const image, const unsigned width, const unsigned height) = 0;

    };

    class OverlayDrawInfo {

    public:
        glm::vec4 m_color{ 1 };
        glm::vec2 m_pos{ 0 }, m_dimension{ 0 };
        glm::vec2 m_texOffset{ 0, 0 };
        glm::vec2 m_texScale{ 1, 1 };
        const OverlayTexture* m_albedoMap = nullptr;
        const OverlayTexture* m_maskMap = nullptr;

    public:
        void flipX(void);
        void flipY(void);

    };

    using overlayDrawFunc_t = std::function<void(const OverlayDrawInfo&, void*)>;

}
