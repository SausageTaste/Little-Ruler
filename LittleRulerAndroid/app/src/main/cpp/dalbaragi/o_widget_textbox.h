#pragma once

#include <array>
#include <string>
#include <glm/glm.hpp>

#include "s_event.h"
#include "o_widgetbase.h"
#include "s_scripting.h"


namespace dal {

    class Label2 : public Widget2 {

    private:
        TextRenderer m_textRenderer;
        glm::vec4 m_backgroundColor;
        touchID_t m_owningTouchID = -1;

    public:
        Label2(Widget2* const parent);

        virtual void render(const UnilocOverlay& uniloc, const float width, const float height) override;

        void setText(const std::string& t) {
            this->m_textRenderer.setText(t);
        }
        const std::string& getText(void) const {
            return this->m_textRenderer.getText();
        }
        void setTextColor(const glm::vec4 color) {
            this->m_textRenderer.setTextColor(color);
        }
        void setBackgroundColor(const glm::vec4 color) {
            this->m_backgroundColor = color;
        }
        void setBackgroundColor(const float x, const float y, const float z, const float w) {
            this->m_backgroundColor = glm::vec4{ x, y, z, w };
        }

    protected:
        virtual void onScrSpaceBoxUpdate(void) override;
        TextRenderer& getTextRenderer(void) {
            return this->m_textRenderer;
        }

    };


    class LineEdit : public Label2 {

    private:
        bool m_onFocus;

    public:
        LineEdit(Widget2* const parent);

        virtual void render(const UnilocOverlay& uniloc, const float width, const float height) override;
        virtual InputCtrlFlag onTouch(const TouchEvent& e) override;
        virtual InputCtrlFlag onKeyInput(const KeyboardEvent& e, const KeyStatesRegistry& keyStates) override;
        virtual void onFocusChange(const bool v) override;

    private:
        void onReturn(void);

    };


    class TextBox : public Widget2 {

    private:
        TextRenderer m_textRenderer;
        StringBufferBasic* m_strBuf;

    public:
        TextBox(Widget2* const parent);

        virtual void render(const UnilocOverlay& uniloc, const float width, const float height) override;
        virtual InputCtrlFlag onTouch(const TouchEvent& e) override;
        virtual InputCtrlFlag onKeyInput(const KeyboardEvent& e, const KeyStatesRegistry& keyStates) override;

        StringBufferBasic* replaceBuffer(StringBufferBasic* const buffer);

    protected:
        virtual void onScrSpaceBoxUpdate(void) override;

    };

}