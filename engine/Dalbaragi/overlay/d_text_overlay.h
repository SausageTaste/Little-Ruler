#pragma once

#include <list>
#include <vector>
#include <string>

#include "d_widget.h"
#include "d_glyph.h"


namespace dal {

    class TextOverlay : public Widget2D {

    private:
        class StrBlock {

        public:
            enum class FollowingType { concat, carriage_return, enter_fmt, exit_fmt };

            class Iterator {

            private:
                const char* m_buf = nullptr;

            public:
                Iterator(void) = default;
                Iterator(const char* const buf);

                bool operator!=(const Iterator& other) const;
                uint32_t operator*(void) const;

                Iterator& operator++(void);

            };

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

            auto begin(void) const {
                return Iterator{ this->m_buf };
            }
            auto end(void) const {
                return Iterator{ this->m_buf + this->m_filledSize };
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
        GlyphMaster* m_glyph;

    public:
        glm::vec4 m_color;
        glm::vec2 m_offset;
        unsigned int m_textSize;
        float m_lineSpacingRate;
        bool m_wordWrap;

    public:
        TextOverlay(Widget2D* const parent, GlyphMaster& glyph, overlayDrawFunc_t drawf);

        virtual void render(const float width, const float height, const void* userdata) override;

        void clear(void);
        void addStr(const char* const str);

    private:
        void render_noWrap(const float width, const float height, const void* userdata) const;

    };

}
