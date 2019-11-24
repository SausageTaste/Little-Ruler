#pragma once

#include "o_widgetbase.h"


namespace dal {

    class TextRenderer : public Widget2 {

    private:
        enum class CharPassFlag { okk, continuee, breakk, carriageReturn };

    public:
        static constexpr size_t cursorNullPos = SIZE_MAX;

    private:
        std::string m_text;
        glm::vec4 m_textColor;
        glm::vec2 m_offset;
        unsigned int m_textSize;
        float m_lineSpacingRate;
        bool m_wordWrap;

    public:
        TextRenderer(Widget2* const parent);

        virtual void render(const UnilocOverlay& uniloc, const float width, const float height) override;

        std::string& textbuf(void) {
            return this->m_text;
        }
        const std::string& textbuf(void) const {
            return this->m_text;
        }

        glm::vec4& textColor(void) {
            return this->m_textColor;
        }
        const glm::vec4& textColor(void) const {
            return this->m_textColor;
        }

        glm::vec2& offset(void) {
            return this->m_offset;
        }
        const glm::vec2& offset(void) const {
            return this->m_offset;
        }

        unsigned& textSize(void) {
            return this->m_textSize;
        }
        const unsigned& textSize(void) const {
            return this->m_textSize;
        }

        const float& lineSpacing(void) const {
            return this->m_lineSpacingRate;
        }
        float& lineSpacing(void) {
            return this->m_lineSpacingRate;
        }

        bool& wordWrap(void) {
            return this->m_wordWrap;
        }
        const bool& wordWrap(void) const {
            return this->m_wordWrap;
        }

    private:
        CharPassFlag isCharQuadInside(glm::vec2& p1, glm::vec2& p2) const;
        std::pair<glm::vec2, glm::vec2> makeCutCharArea(glm::vec2 p1, glm::vec2 p2);

    };


    class TextScroll : public TextRenderer {

    private:
        glm::vec2 m_lastTouchPos;
        touchID_t m_owning = -1;

    public:
        TextScroll(Widget2* const parent);

        virtual InputCtrlFlag onTouch(const TouchEvent& e) override;

    };

}
