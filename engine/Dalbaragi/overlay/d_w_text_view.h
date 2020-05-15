#pragma once

#include "d_text_overlay.h"
#include "d_widget_view.h"


namespace dal {

    class Lable : public Widget2D {

    private:
        TextOverlay m_text;
        ColorView m_bg;

        float m_margin;

    public:
        Lable(Widget2D* const parent, overlayDrawFunc_t drawf, GlyphMaster& glyph);

        virtual void render(const float width, const float height, const void* userdata) override;
        virtual void onUpdateAABB(void) override;

        auto text(void) const -> std::string {
            return this->m_text.textWhole();
        }

        void setText(const char* const str) {
            this->m_text.clear();
            this->m_text.addStr(str);
        }
        void addText(const char* const str) {
            this->m_text.addStr(str);
        }
        void addChar(const char c) {
            this->m_text.addAscii(c);
        }
        void backspace(void) {
            this->m_text.backspace();
        }

        void setMargin(const float v) {
            this->m_margin = v;
        }
        void setTextColor(const float r, const float g, const float b, const float a = 1) {
            this->m_text.m_color.r = r;
            this->m_text.m_color.g = g;
            this->m_text.m_color.b = b;
            this->m_text.m_color.a = a;
        }
        void setTextColor(const glm::vec4& color) {
            this->m_text.m_color = color;
        }
        void setBGColor(const float r, const float g, const float b, const float a = 1) {
            this->m_bg.m_color.r = r;
            this->m_bg.m_color.g = g;
            this->m_bg.m_color.b = b;
            this->m_bg.m_color.a = a;
        }
        void setBGColor(const glm::vec4& color) {
            this->m_bg.m_color = color;
        }

    };


    class LineEdit : public Lable {

    private:
        std::function<void(const char* const)> m_onReturn;

    public:
        using Lable::Lable;

        virtual auto onTouch(const TouchEvent& e)->InputDealtFlag override;
        virtual auto onKeyInput(const KeyboardEvent& e, const KeyStatesRegistry& keyStates)->InputDealtFlag override;

        void setCallback_onReturn(std::function<void(const char* const)> func) {
            this->m_onReturn = func;
        }

    private:
        void onReturn(void);

    };


    class TextBox : public Widget2D {

    public:
        TextOverlay m_text;
        ColorView m_bg;
        float m_margin;

        touchID_t m_owning = -1;
        glm::vec2 m_lastTouchPos;

    public:
        TextBox(Widget2D* const parent, overlayDrawFunc_t drawf, GlyphMaster& glyph);

        virtual void render(const float width, const float height, const void* userdata) override;
        virtual void onUpdateAABB(void) override;

        virtual auto onTouch(const TouchEvent& e)->InputDealtFlag override;

    };

}
