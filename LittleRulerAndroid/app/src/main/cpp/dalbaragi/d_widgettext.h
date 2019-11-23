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
        glm::vec2 m_offset, m_lastTouchPos;
        Timer m_cursorTimer;
        size_t m_cursorPos;
        unsigned int m_textSize;
        float m_lineSpacingRate;
        bool m_wordWrap;
        touchID_t m_owning;

    public:
        TextRenderer(Widget2* const parent);

        virtual void render(const UnilocOverlay& uniloc, const float width, const float height) override;
        virtual InputCtrlFlag onTouch(const TouchEvent& e) override;

        const std::string& getText(void) const {
            return this->m_text;
        }
        void setText(const std::string& t) {
            this->m_text = t;
        }
        void setText(std::string&& t) {
            this->m_text = std::move(t);
        }

        void appendText(const std::string& t) {
            this->m_text.append(t);
        }
        void appendText(const char c) {
            this->m_text += c;
        }
        void popBackText(void) {
            this->m_text.pop_back();
        }

        const glm::vec4& getTextColor(void) const {
            return this->m_textColor;
        }
        void setTextColor(const float x, const float y, const float z, const float w) {
            this->m_textColor.x = x;
            this->m_textColor.y = y;
            this->m_textColor.z = z;
            this->m_textColor.w = w;
        }
        void setTextColor(const glm::vec4& v) {
            this->m_textColor = v;
        }

        const glm::vec2& getOffset(void) const {
            return this->m_offset;
        }
        void setOffset(const float x, const float y) {
            this->m_offset.x = x;
            this->m_offset.y = y;
        }
        void setOffset(const glm::vec2& v) {
            this->m_offset = v;
        }

        void setCursorPos(const size_t pos) {
            this->m_cursorPos = pos;
        }

        unsigned int getTextSize(void) const {
            return this->m_textSize;
        }
        void setTextSize(const unsigned int v) {
            this->m_textSize = v;
        }

        float getLineSpacingRate(void) const {
            return this->m_lineSpacingRate;
        }
        void setLineSpacingRate(const float v) {
            this->m_lineSpacingRate = v;
        }

        bool isWordWrap(void) const {
            return this->m_wordWrap;
        }
        void setWordWrap(const bool v) {
            this->m_wordWrap = v;
        }

    private:
        bool canDrawCursor(void);
        CharPassFlag isCharQuadInside(glm::vec2& p1, glm::vec2& p2) const;
        void makeOffsetApproch(void);
        std::pair<glm::vec2, glm::vec2> makeCutCharArea(glm::vec2 p1, glm::vec2 p2);

    };

}
