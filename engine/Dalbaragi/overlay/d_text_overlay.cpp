#include "d_text_overlay.h"

#include <cassert>
#include <memory>
#include <unordered_map>

#include <fmt/format.h>
extern "C" {
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
}

#include "d_overlay_base.h"


// Global values
namespace {

    constexpr float TAP_CHAR_WIDTH = 20.0f;

}


// Copied functions
namespace {

    // from https://github.com/s22h/cutils/blob/master/include/s22h/unicode.h

    constexpr uint32_t UTF8_ONE_BYTE_MASK = 0x80;
    constexpr uint32_t UTF8_ONE_BYTE_BITS = 0;
    constexpr uint32_t UTF8_TWO_BYTES_MASK = 0xE0;
    constexpr uint32_t UTF8_TWO_BYTES_BITS = 0xC0;
    constexpr uint32_t UTF8_THREE_BYTES_MASK = 0xF0;
    constexpr uint32_t UTF8_THREE_BYTES_BITS = 0xE0;
    constexpr uint32_t UTF8_FOUR_BYTES_MASK = 0xF8;
    constexpr uint32_t UTF8_FOUR_BYTES_BITS = 0xF0;
    constexpr uint32_t UTF8_CONTINUATION_MASK = 0xC0;
    //constexpr uint32_t UTF8_CONTINUATION_BITS = 0x80;

    constexpr unsigned int MAX_UTF8_CODE_SIZE = 4;

    // Returns [0, MAX_UTF8_CODE_SIZE]
    size_t utf8_codepoint_size(const uint8_t byte) {
        if ( (byte & UTF8_ONE_BYTE_MASK) == UTF8_ONE_BYTE_BITS ) {
            return 1;
        }

        if ( (byte & UTF8_TWO_BYTES_MASK) == UTF8_TWO_BYTES_BITS ) {
            return 2;
        }

        if ( (byte & UTF8_THREE_BYTES_MASK) == UTF8_THREE_BYTES_BITS ) {
            return 3;
        }

        if ( (byte & UTF8_FOUR_BYTES_MASK) == UTF8_FOUR_BYTES_BITS ) {
            return 4;
        }

        return 0;
    }

    uint32_t convertUTF8to32(const uint8_t* arr) {
        const auto cp_size = utf8_codepoint_size(*arr);

        uint32_t buffer = 0;

        switch ( cp_size ) {

        case 1:
            buffer = ((uint32_t)arr[0] & ~UTF8_ONE_BYTE_MASK);
            return buffer;
        case 2:
            buffer =
                ((uint32_t)arr[0] & ~UTF8_TWO_BYTES_MASK) << 6 |
                ((uint32_t)arr[1] & ~UTF8_CONTINUATION_MASK);
            return buffer;
        case 3:
            buffer =
                ((uint32_t)arr[0] & ~UTF8_THREE_BYTES_MASK) << 12 |
                ((uint32_t)arr[1] & ~UTF8_CONTINUATION_MASK) << 6 |
                ((uint32_t)arr[2] & ~UTF8_CONTINUATION_MASK);
            return buffer;
        case 4:
            buffer =
                ((uint32_t)arr[0] & ~UTF8_FOUR_BYTES_MASK) << 18 |
                ((uint32_t)arr[1] & ~UTF8_CONTINUATION_MASK) << 12 |
                ((uint32_t)arr[2] & ~UTF8_CONTINUATION_MASK) << 6 |
                ((uint32_t)arr[3] & ~UTF8_CONTINUATION_MASK);
            return buffer;
        default:
            // this should never happen, since we check validity of the string
            //dalAbort("utf8_to_utf32: invalid byte in UTF8 string.\n");
            return 0;

        }
    }

}


// Util functions
namespace {

    using vec2Pair_t = std::pair<glm::vec2, glm::vec2>;

    vec2Pair_t calcScaleOffset(const vec2Pair_t& larger, const vec2Pair_t& smaller) {
        const auto lWidth = larger.second.x - larger.first.x;
        const auto lHeight = larger.second.y - larger.first.y;
        const auto sWidth = smaller.second.x - smaller.first.x;
        const auto sHeight = smaller.second.y - smaller.first.y;

        const glm::vec2 scale{ sWidth / lWidth , sHeight / lHeight };

        // IDK why but I need to assume that origin of tex coords is upper left.
        const glm::vec2 lPoint01{ larger.first };
        const glm::vec2 sPoint01{ smaller.first };

        const glm::vec2 offset{ (sPoint01.x - lPoint01.x) / lWidth, (sPoint01.y - lPoint01.y) / lHeight };

        return std::make_pair(scale, offset);
    }

    template <char _Criteria, typename _Iter>
    _Iter findNextChar(_Iter beg, const _Iter end) {
        for ( ; beg != end; ++beg ) {
            const auto c = *beg;
            if ( c == _Criteria ) {
                return beg;
            }
        }

        return end;
    }

}


// StrBlock and its iterator
namespace dal {

    TextOverlay::StrBlock::Iterator::Iterator(const char* const buf)
        : m_buf(buf)
    {

    }

    bool TextOverlay::StrBlock::Iterator::operator!=(const Iterator& other) const {
        return this->m_buf != other.m_buf;
    }

    uint32_t TextOverlay::StrBlock::Iterator::operator*(void) const {
        const auto ch = static_cast<uint8_t>(*this->m_buf);
        const auto codeSize = utf8_codepoint_size(ch);
        assert(codeSize <= MAX_UTF8_CODE_SIZE);

        if ( codeSize > 1 ) {
            return convertUTF8to32(reinterpret_cast<const uint8_t*>(this->m_buf));
        }
        else {
            return ch;
        }
    }

    TextOverlay::StrBlock::Iterator& TextOverlay::StrBlock::Iterator::operator++(void) {
        const auto ch = static_cast<uint8_t>(*this->m_buf);
        const auto codeSize = utf8_codepoint_size(ch);
        this->m_buf += codeSize;
        return *this;
    }


    void TextOverlay::StrBlock::clearBuf(void) {
        this->m_filledSize = 0;
        this->m_buf[this->m_filledSize] = '\0';
    }

    void TextOverlay::StrBlock::pushChar(const char c) {
        static_assert(sizeof(StrBlock) == BLOCK_SIZE);
        static_assert(MAX_UTF8_CODE_SIZE <= BUF_SIZE);

        assert('\0' != c);
        assert(0 != this->remainingCap());

        this->m_buf[this->m_filledSize] = c;
        ++this->m_filledSize;
        this->m_buf[this->m_filledSize] = '\0';
    }

    bool TextOverlay::StrBlock::pushStr(const char* const str, size_t size) {
        if ( this->remainingCap() < size ) {
            return false;
        }

        std::memcpy(this->m_buf + this->m_filledSize, str, size);
        this->m_filledSize += size;
        this->m_buf[this->m_filledSize] = '\0';

        return true;
    }

}



namespace {

    enum class CharSkipFlag { skip, dont_skip, finish, carriage_return };

    CharSkipFlag checkCharSkip(const dal::AABB_2D<float>& aabb, const glm::vec2& p1, const glm::vec2& p2) {
        const auto pp11 = aabb.point11();

        if ( p1.x > pp11.x ) {
            // This shouldn't happen because carriage return already dealt in render function body.
            return CharSkipFlag::carriage_return;
        }
        else if ( p1.y > pp11.y ) {
            return CharSkipFlag::skip;
        }
        else if ( p2.x < aabb.pos().x ) {
            return CharSkipFlag::skip;
        }
        else if ( p2.y < aabb.pos().y ) {
            return CharSkipFlag::skip;
        }
        else {
            return CharSkipFlag::dont_skip;
        }
    }

    std::pair<glm::vec2, glm::vec2> makeCutCharArea(const dal::AABB_2D<float>& aabb, glm::vec2 p1, glm::vec2 p2) {
        {
            const auto p00 = aabb.point00();

            if ( p1.x < p00.x ) {
                p1.x = p00.x;
            }
            if ( p1.y < p00.y ) {
                p1.y = p00.y;
            }
        }

        {
            const auto pp11 = aabb.point11();

            if ( p2.x > pp11.x ) {
                p2.x = pp11.x;
            }
            if ( p2.y > pp11.y ) {
                p2.y = pp11.y;
            }
        }

        return std::make_pair(p1, p2);
    }

}



namespace dal {

    TextOverlay::TextOverlay(Widget2D* const parent, GlyphMaster& glyph, overlayDrawFunc_t drawf)
        : Widget2D(parent, drawf)
        , m_glyph(&glyph)
        , m_color(1, 1, 1, 1)
        , m_textSize(15)
        , m_lineSpacingRate(1.3)
        , m_wordWrap(false)
    {
        this->m_blocks.emplace_back();
    }

    void TextOverlay::render(const float width, const float height, const void* userdata) {
        this->render_noWrap(width, height, userdata);
    }

    void TextOverlay::clear(void) {
        this->m_blocks.clear();
        this->m_blocks.emplace_back();
    }

    void TextOverlay::addStr(const char* const str) {
        const char* iter = str;

        while ( '\0' != *iter ) {
            const auto c = *iter;
            const auto codeSize = utf8_codepoint_size(c);

            if ( 1 == codeSize ) {
                if ( '\n' == c ) {
                    auto& added = this->m_blocks.emplace_back();
                    added.setType(StrBlock::FollowingType::carriage_return);
                }
                else {
                    // Ensure last block has at least 1 capacity.
                    if ( 0 == this->m_blocks.back().remainingCap() ) {
                        this->m_blocks.emplace_back();
                    }

                    this->m_blocks.back().pushChar(c);
                }
            }
            else if ( 0 == codeSize ) {
                assert(false && "Code size if 0, which is not supposed to happen.");
            }
            else {
                // Ensure last block has enough capacity.
                if ( this->m_blocks.back().remainingCap() < codeSize ) {
                    this->m_blocks.emplace_back();
                }

                this->m_blocks.back().pushStr(iter, codeSize);
            }

            iter += codeSize;
        }
    }

    // Private
    void TextOverlay::render_noWrap(const float width, const float height, const void* userdata) const {
        const auto p11 = this->aabb().point11();
        const auto leftMostPos = this->aabb().pos().x + this->m_offset.x;

        glm::vec2 currentPos{ leftMostPos, this->aabb().pos().y + static_cast<float>(this->m_textSize) + this->m_offset.y };
        bool skipUntilNewLine = false;

        for ( auto& block : this->m_blocks ) {
            if ( skipUntilNewLine ) {
                if ( StrBlock::FollowingType::carriage_return != block.type() )
                    continue;
                else
                    skipUntilNewLine = false;
            }

            if ( StrBlock::FollowingType::carriage_return == block.type() ) {
                currentPos.x = leftMostPos;
                currentPos.y += static_cast<float>(this->m_textSize) * this->m_lineSpacingRate;
            }

            // For each char
            for ( const auto c : block ) {
                // Process control char
                {
                    if ( ' ' == c ) {
                        currentPos.x += this->m_glyph->get(c, this->m_textSize).m_advance >> 6;
                        continue;
                    }
                    else if ( '\t' == c ) {
                        currentPos.x += TAP_CHAR_WIDTH;
                        continue;
                    }
                }

                auto& charInfo = this->m_glyph->get(c, this->m_textSize);

                // Skip to next line if current line is full.
                if ( currentPos.x >= p11.x ) {
                    skipUntilNewLine = true;
                    continue;
                }

                std::pair<glm::vec2, glm::vec2> charQuad;
                {
                    charQuad.first.x = roundf(currentPos.x + charInfo.m_bearing.x);
                    charQuad.first.y = roundf(currentPos.y - charInfo.m_bearing.y);
                    charQuad.second.x = charQuad.first.x + static_cast<float>(charInfo.m_size.x);
                    charQuad.second.y = charQuad.first.y + static_cast<float>(charInfo.m_size.y);
                }

                switch ( checkCharSkip(this->aabb(), charQuad.first, charQuad.second) ) {

                case CharSkipFlag::finish:
                    return;
                case CharSkipFlag::skip:
                    currentPos.x += (charInfo.m_advance >> 6);
                    continue;
                case CharSkipFlag::carriage_return:
                    skipUntilNewLine = true;
                    continue;

                }

                {
                    const auto charQuadCut = makeCutCharArea(this->aabb(), charQuad.first, charQuad.second);
                    OverlayDrawInfo charQuadInfo;

                    const glm::vec2 bottomLeft{ charQuadCut.first.x, charQuadCut.second.y };
                    const auto recSize = glm::vec2{
                        charQuadCut.second.x - charQuadCut.first.x,
                        charQuadCut.second.y - charQuadCut.first.y
                    };

                    charQuadInfo.m_pos = screen2device(bottomLeft, width, height);
                    charQuadInfo.m_dimension = size2device(recSize, glm::vec2{ width, height });

                    charQuadInfo.m_color = this->m_color;
                    charQuadInfo.m_maskMap = &*charInfo.m_tex;

                    if ( charQuadCut.first != charQuad.first || charQuadCut.second != charQuad.second ) {
                        const auto [scale, offset] = calcScaleOffset(charQuad, charQuadCut);

                        charQuadInfo.m_texScale = scale;
                        charQuadInfo.m_texOffset = offset;
                    }
                    charQuadInfo.flipY();

                    this->draw(charQuadInfo, userdata);
                }

                currentPos.x += (charInfo.m_advance >> 6);
            }
        }
    }

}
