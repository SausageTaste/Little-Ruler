#pragma once

#include <d_input_data.h>


namespace dal {

    enum class InputDealtFlag { ignored, consumed, owned };


    class Widget {

    private:
        Widget* m_parent = nullptr;

    public:
        Widget(const Widget&) = delete;
        Widget& operator=(const Widget&) = delete;

    public:
        Widget(Widget* const parent)
            : m_parent(parent)
        {

        }

        Widget(Widget&&) = default;
        Widget& operator=(Widget&&) = default;
        virtual ~Widget(void) = default;

        auto parent(void) -> Widget* {
            return this->m_parent;
        }
        auto parent(void) const -> const Widget* {
            return this->m_parent;
        }

        virtual void render(const float width, const float height, void* userData) {}
        virtual auto onTouch(const TouchEvent& e) -> InputDealtFlag {
            return InputCtrlFlag::ignored;
        }
        virtual auto onKeyInput(const KeyboardEvent& e, const KeyStatesRegistry& keyStates) -> InputDealtFlag {
            return InputCtrlFlag::ignored;
        }
        virtual void onParentResize(const float width, const float height) {}
        virtual void onFocusChange(const bool v) {}

    };

}
