#pragma once

#include <array>
#include <string>
#include <glm/glm.hpp>

#include "o_widget_primitive.h"
#include "s_event.h"
#include "o_text_cache.h"
#include "o_widget_base.h"
#include "s_scripting.h"


namespace dal {

    class Label : public Widget {

    private:
        QuadRenderer m_background;
        std::string m_text;
        glm::vec4 m_textColor;
        UnicodeCache& m_unicodeCache;

    public:
        Label(Widget* parent, UnicodeCache& asciiCache);

        virtual void onClick(const float x, const float y) override;
        virtual void renderOverlay(const UnilocOverlay& uniloc) override;

        void setText(const std::string& t);
        const std::string& getText(void) const;
        void setTextColor(const float r, const float g, const float b, const float a);
        void setBackgroundColor(const float r, const float g, const float b, const float a);

    };


    class LineEdit : public Label {

    public:
        LineEdit(Widget* parent, UnicodeCache& asciiCache);
        void onReturn(void);

        virtual void onClick(const float x, const float y) override;
        virtual void onKeyInput(const char* const c) override;
        virtual void onFocusChange(bool isFocus) override;

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