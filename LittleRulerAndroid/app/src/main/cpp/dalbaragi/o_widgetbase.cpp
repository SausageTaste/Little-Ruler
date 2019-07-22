#include "o_widgetbase.h"

#include <unordered_map>

#include <fmt/format.h>

#include "s_logger_god.h"
#include "s_freetype_master.h"
#include "p_resource.h"


using namespace fmt::literals;


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
    constexpr uint32_t UTF8_CONTINUATION_BITS = 0x80;
    
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

    uint32_t convert_utf8_to_utf32(const uint8_t* begin, const uint8_t* end) {
        const auto cp_size = end - begin;
        assert((utf8_codepoint_size(*begin)) == cp_size);
        assert(1 <= cp_size && cp_size <= 4);

        uint32_t buffer = 0;

        switch ( cp_size ) {

        case 1:
            buffer = ((uint32_t)begin[0] & ~UTF8_ONE_BYTE_MASK);
            return buffer;
        case 2:
            buffer =
                ((uint32_t)begin[0] & ~UTF8_TWO_BYTES_MASK) << 6 |
                ((uint32_t)begin[1] & ~UTF8_CONTINUATION_MASK);
            return buffer;
        case 3:
            buffer =
                ((uint32_t)begin[0] & ~UTF8_THREE_BYTES_MASK) << 12 |
                ((uint32_t)begin[1] & ~UTF8_CONTINUATION_MASK) << 6 |
                ((uint32_t)begin[2] & ~UTF8_CONTINUATION_MASK);
            return buffer;
        case 4:
            buffer =
                ((uint32_t)begin[0] & ~UTF8_FOUR_BYTES_MASK) << 18 |
                ((uint32_t)begin[1] & ~UTF8_CONTINUATION_MASK) << 12 |
                ((uint32_t)begin[2] & ~UTF8_CONTINUATION_MASK) << 6 |
                ((uint32_t)begin[3] & ~UTF8_CONTINUATION_MASK);
            return buffer;
        default:
            // this should never happen, since we check validity of the string
            fprintf(stderr, "utf8_to_utf32: invalid byte in UTF8 string.\n");
            return 0;

        }
    }

}


namespace {

    class RealQuadRenderer2 {

    private:
        GLuint mBufferObj;
        GLuint mVao;

        RealQuadRenderer2(void) {
            glGenVertexArrays(1, &mVao);
            if ( mVao <= 0 ) {
                dalAbort("Failed to generate vertex array.");
            }
            glGenBuffers(1, &mBufferObj);
            if ( mBufferObj <= 0 ) {
                dalAbort("Failed to generate gl buffers.");
            }

            glBindVertexArray(mVao);

            /* Vertices */
            {
                GLfloat vertices[12] = {
                        0, 1,
                        0, 0,
                        1, 0,
                        0, 1,
                        1, 0,
                        1, 1
                };
                auto size = 12 * sizeof(float);

                glBindBuffer(GL_ARRAY_BUFFER, this->mBufferObj);
                glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
            }

            glBindVertexArray(0);
        }

    public:
        static RealQuadRenderer2& getinst(void) {
            static RealQuadRenderer2 inst;
            return inst;
        }

        void drawArray(void) {
            glBindVertexArray(this->mVao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

    };


    class UnicodeCharCache {

    public:
        struct CharacterUnit {
            glm::ivec2    size;
            glm::ivec2    bearing;
            dal::Texture* tex = nullptr;
            int32_t       advance = 0;
        };

    private:
        using utf32ToTexMap_t = std::unordered_map<uint32_t, CharacterUnit>;
        // This is map from font size to another map.
        using textSizeToSubmap_t = std::unordered_map<unsigned int, utf32ToTexMap_t>;

        textSizeToSubmap_t m_cache;

    public:
        const CharacterUnit& at(const uint32_t utf32Char, unsigned int textSize) {
            auto& submap = this->findOrCreateSubmap(textSize, this->m_cache);
            const auto found = submap.find(utf32Char);

            if ( submap.end() != found ) {
                return found->second;
            }
            else {
                auto [added, success] = submap.emplace(utf32Char, CharacterUnit{});
                if ( false == success ) {
                    dalAbort("Failed to add a glyph");
                }

                auto& charUnit = added->second;
                this->fillData(charUnit, utf32Char, textSize);
                return charUnit;
            }
        }

    private:
        utf32ToTexMap_t& findOrCreateSubmap(const unsigned int textSize, textSizeToSubmap_t& cache) const {
            auto found = cache.find(textSize);

            if ( cache.end() != found ) {
                return found->second;
            }
            else {
                auto emplaced = cache.emplace(textSize, utf32ToTexMap_t{});
                dalAssert(emplaced.second);
                return emplaced.first->second;
            }
        }

        void fillData(CharacterUnit& charUnit, const uint32_t utf32Char, unsigned int textSize) {
            auto& ftype = dal::FreetypeGod::getinst();

            ftype.setFontSize(textSize);
            auto& face = ftype.getFace();
            const auto glyphIndex = FT_Get_Char_Index(face, utf32Char);
            if ( FT_Load_Glyph(face, glyphIndex, FT_LOAD_RENDER) != 0 ) {
                dalAbort("Failed to load Glyph for utf-32 char: {}"_format(utf32Char));
            }

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // Text looks broken without this.
            charUnit.tex = dal::ResourceMaster::getUniqueTexture();
            charUnit.tex->init_maskMap(face->glyph->bitmap.buffer, face->glyph->bitmap.width, face->glyph->bitmap.rows);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

            charUnit.size = glm::vec2{ face->glyph->bitmap.width, face->glyph->bitmap.rows };
            charUnit.bearing = glm::vec2{ face->glyph->bitmap_left, face->glyph->bitmap_top };
            charUnit.advance = static_cast<int32_t>(face->glyph->advance.x);
        }

    } g_charCache;

}


namespace dal {

    glm::vec2 screen2device(const glm::vec2& p, const float winWidth, const float winHeight) {
        return glm::vec2{
             2.0f * p.x / (winWidth) - 1.0f,
            -2.0f * p.y / (winHeight) + 1.0f
        };
    }

    glm::vec2 screen2device(const glm::vec2& p, const unsigned int winWidth, const unsigned int winHeight) {
        return screen2device(p, static_cast<float>(winWidth), static_cast<float>(winHeight));
    }

    glm::vec2 device2screen(const glm::vec2& p, const float winWidth, const float winHeight) {
        return glm::vec2{
            (p.x + 1.0f) * (winWidth) / 2.0f,
            (1.0f - p.y) * (winHeight) / 2.0f
        };
    }

    glm::vec2 device2screen(const glm::vec2& p, const unsigned int winWidth, const unsigned int winHeight) {
        return device2screen(p, static_cast<float>(winWidth), static_cast<float>(winHeight));
    }

    void renderQuadOverlay(const UnilocOverlay& uniloc, const glm::vec2& devSpcP1, const glm::vec2& devSpcP2, const glm::vec4& color,
        const Texture* const diffuseMap, const Texture* const maskMap, const bool upsideDown_diffuseMap, const bool upsideDown_maskMap)
    {
        uniloc.point1(devSpcP1);
        uniloc.point2(devSpcP2);
        uniloc.color(color);
        uniloc.upsideDownDiffuseMap(upsideDown_diffuseMap);
        uniloc.upsideDownMaskMap(upsideDown_maskMap);

        if ( nullptr != diffuseMap ) {
            diffuseMap->sendUniform(uniloc.getDiffuseMap());
        }
        else {
            uniloc.getDiffuseMap().setFlagHas(false);
        }

        if ( nullptr != maskMap ) {
            maskMap->sendUniform(uniloc.getMaskMap());
        }
        else {
            uniloc.getMaskMap().setFlagHas(false);
        }

        RealQuadRenderer2::getinst().drawArray();
    }

    void renderQuadOverlay(const UnilocOverlay& uniloc, const std::pair<glm::vec2, glm::vec2>& devSpc, const glm::vec4& color,
        const Texture* const diffuseMap, const Texture* const maskMap, const bool upsideDown_diffuseMap, const bool upsideDown_maskMap)
    {
        renderQuadOverlay(uniloc, devSpc.first, devSpc.second, color, diffuseMap, maskMap, upsideDown_diffuseMap, upsideDown_maskMap);
    }

}


// ScreenSpaceBox
namespace dal {

    bool IScreenSpaceBox::isPointInside(const glm::vec2& v) const {
        return this->isPointInside(v.x, v.y);
    }

    bool IScreenSpaceBox::isPointInside(const float x, const float y) const {
        const auto p1 = this->getPoint00();
        const auto p2 = this->getPoint11();

        if ( x < p1.x ) {
            return false;
        }
        else if ( y < p1.y ) {
            return false;
        }
        else if ( x > p2.x ) {
            return false;
        }
        else if ( y > p2.y ) {
            return false;
        }
        else {
            return true;
        }
    }

    std::pair<glm::vec2, glm::vec2> IScreenSpaceBox::makeDeviceSpace(const float width, const float height) const {
        std::pair<glm::vec2, glm::vec2> result;

        result.first = screen2device(this->getPoint01(), width, height);
        result.second = screen2device(this->getPoint10(), width, height);

        dalAssert(result.first.x <= result.second.x);
        dalAssert(result.first.y <= result.second.y);

        return result;
    }

}


// Widget2
namespace dal {

    Widget2::Widget2(Widget2* const parent)
        : m_parent(parent)
        , m_flagDraw(true)
    {

    }

}


namespace dal {

    TextRenderer::TextRenderer(Widget2* const parent)
        : Widget2(parent)
        , m_textSize(15)
        , m_lineSpacingRate(1.2f)
        , m_wordWrap(false)
    {

    }

    void TextRenderer::render(const UnilocOverlay& uniloc, const float width, const float height) {
        const auto parentP2 = this->getPoint11();

        const float xInit = this->getPosX() + this->m_offset.x;
        float xAdvance = xInit;
        float yHeight = this->getPosY() + static_cast<float>(this->m_textSize) + this->m_offset.y;

        auto header = this->m_text.begin();
        const auto end = this->m_text.end();

        while ( end != header ) {
            uint32_t c = 0;
            {
                const auto ch = static_cast<uint8_t>(*header++);
                const auto codeSize = utf8_codepoint_size(ch);
                dalAssert(codeSize <= MAX_UTF8_CODE_SIZE);
                if ( codeSize > 1 ) {
                    uint8_t buffer[MAX_UTF8_CODE_SIZE];
                    for ( size_t i = 0; i < codeSize; i++ ) {
                        buffer[i] = ch;
                    }

                    c = convert_utf8_to_utf32(&buffer[0], &buffer[0] + codeSize);
                }
                else {
                    c = ch;
                }
            }

            if ( '\n' == c ) {
                yHeight += static_cast<float>(this->m_textSize) * this->m_lineSpacingRate;
                xAdvance = xInit;
                continue;
            }
            else if ( '\t' == c ) {
                xAdvance += TAP_CHAR_WIDTH;
                continue;
            }

            auto& charInfo = g_charCache.at(c, this->m_textSize);

            // Carriage return if current line is full.
            if ( xAdvance + charInfo.size.x >= parentP2.x ) {
                if ( this->m_wordWrap ) {
                    yHeight += static_cast<float>(this->m_textSize) * this->m_lineSpacingRate;
                    xAdvance = xInit;
                }
                else {
                    continue;
                }
            }

            std::pair<glm::vec2, glm::vec2> charQuad;

            charQuad.first.x = xAdvance + charInfo.bearing.x;
            charQuad.first.y = yHeight - charInfo.bearing.y;
            if ( charQuad.first.y < this->getPosY() ) {
                continue;
            }

            charQuad.second.x = charQuad.first.x + charInfo.size.x;
            charQuad.second.y = charQuad.first.y + charInfo.size.y;
            if ( charQuad.second.y > parentP2.y ) {
                return;
            }

            std::pair<glm::vec2, glm::vec2> deviceSpace = std::make_pair(
                screen2device(charQuad.first, width, height),
                screen2device(charQuad.second, width, height)
            );
            renderQuadOverlay(uniloc, deviceSpace, { 1.0, 1.0, 1.0, 1.0 }, nullptr, charInfo.tex, false, false);

            xAdvance += (charInfo.advance >> 6);
        }
    }

}