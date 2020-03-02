#pragma once

#include <list>

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


namespace dal {

    class TextRenderer2 : public Widget2 {

    private:
        class StrBlock {

        public:
            enum class FollowingType{ concat, carriage_return, enter_fmt, exit_fmt };

        private:
            static constexpr size_t BLOCK_SIZE = 64;
            static constexpr size_t BUF_SIZE = BLOCK_SIZE - sizeof(FollowingType) - sizeof(unsigned);

        private:
            char m_buf[BUF_SIZE] = { '\0' };
            unsigned m_filledSize = 0;
            FollowingType m_type = FollowingType::concat;

        public:
            static constexpr unsigned capacity(void) {
                return BUF_SIZE - 1;
            }

            unsigned size(void) const {
                return this->m_filledSize;
            }
            unsigned remainingCap(void) const {
                return this->capacity() - this->size();
            }
            FollowingType type(void) const {
                return this->m_type;
            }
            const char* buffer(void) const {
                return this->m_buf;
            }

            void clearBuf(void);
            void pushChar(const char c);
            bool pushStr(const char* const str, size_t size);

            void setType(const FollowingType t) {
                this->m_type = t;
            }

        };

    private:
        std::list<StrBlock> m_blocks;

        glm::vec4 m_color;
        glm::vec2 m_offset;
        unsigned int m_textSize;
        float m_lineSpacingRate;
        bool m_wordWrap;

    public:
        TextRenderer2(Widget2* const parent);

        void addStr(const char* const str);

    };

}
