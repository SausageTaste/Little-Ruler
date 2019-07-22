#pragma once

#include <array>
#include <string>
#include <glm/glm.hpp>

#include "o_widget_primitive.h"
#include "s_event.h"
#include "o_text_cache.h"
#include "o_widget_base.h"
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
        virtual InputCtrlFlag onKeyInput(const KeyboardEvent& e, const KeyAdditionalStates& additional) override;
        virtual void onFocusChange(const bool v) override;

    private:
        void onReturn(void);

    };


    class TextStream : public LuaStdOutput {

    private:
        std::array<char, 1024> m_buffer;
        unsigned int m_topIndex = 0;

    public:
        virtual bool append(const char* const str) override;
        bool append(const std::string& str);
        const char* get(void);
        void clear(void);

        unsigned int getSize(void) const;
        unsigned int getReserved(void) const;

    private:
        bool append(const char* const ptr, const size_t size);

    };


    class TextBox : public Widget {

    private:
        TextStream* m_strBuffer = nullptr;
        std::string m_text;
        std::vector<char32_t> m_utf32Str;
        QuadRenderer m_quadRender;
        UnicodeCache& m_unicodes;

        int m_scroll = 0;

    public:
        explicit TextBox(Widget* parent, UnicodeCache& unicodes);
        TextStream* setStrBuf(TextStream* const strBuf);

        virtual void onClick(const float x, const float y) override;
        virtual void renderOverlay(const UnilocOverlay& uniloc) override;

        void setColor(float r, float g, float b, float a) {
            this->m_quadRender.setColor(r, g, b, a);
        }

        int addScroll(int v);

    private:
        void fetchStream(void);

    };

}