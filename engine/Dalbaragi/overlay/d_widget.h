#pragma once

#include <d_overlay_base.h>
#include <d_input_data.h>
#include <d_aabb_2d.h>


namespace dal {

    enum class InputDealtFlag { ignored, consumed, owned };


    class Widget {

    public:
        Widget(const Widget&) = delete;
        Widget& operator=(const Widget&) = delete;

    public:
        Widget(void) = default;
        Widget(Widget&&) = default;
        Widget& operator=(Widget&&) = default;
        virtual ~Widget(void) = default;

        virtual void render(const float width, const float height, const void* userdata) {}
        virtual auto onTouch(const TouchEvent& e) -> InputDealtFlag {
            return InputDealtFlag::ignored;
        }
        virtual auto onKeyInput(const KeyboardEvent& e, const KeyStatesRegistry& keyStates) -> InputDealtFlag {
            return InputDealtFlag::ignored;
        }
        virtual void onFocusChange(const bool v) {}

    };

    class Widget2D : public Widget {

    private:
        AABB_2D<float> m_aabb;
        Widget2D* m_parent = nullptr;
        overlayDrawFunc_t m_drawfunc;

    public:
        Widget2D(Widget2D* const parent, overlayDrawFunc_t drawf)
            : m_parent(parent)
            , m_drawfunc(drawf)
        {

        }

        virtual void onUpdateDimens(const float scale) {};

        auto aabb(void) -> AABB_2D<float>& {
            return this->m_aabb;
        }
        auto aabb(void) const -> const AABB_2D<float> {
            return this->m_aabb;
        }

    protected:
        void draw(const OverlayDrawInfo& info, const void* userdata) const {
            if ( this->m_drawfunc )
                this->m_drawfunc(info, userdata);
        }

    };


    glm::vec2 screen2device(const glm::vec2& p, const float winWidth, const float winHeight);
    glm::vec2 screen2device(const glm::vec2& p, const unsigned int winWidth, const unsigned int winHeight);
    glm::vec2 device2screen(const glm::vec2& p, const float winWidth, const float winHeight);
    glm::vec2 device2screen(const glm::vec2& p, const unsigned int winWidth, const unsigned int winHeight);
    glm::vec2 size2device(const glm::vec2& size, const glm::vec2& parentSize);

}
