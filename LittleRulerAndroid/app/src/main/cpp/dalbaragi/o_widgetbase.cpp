#include "o_widgetbase.h"

#include <unordered_map>

#include <fmt/format.h>
extern "C" {
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
}

#include "s_logger_god.h"
#include "p_resource.h"
#include "u_fileclass.h"


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
            dalAbort("utf8_to_utf32: invalid byte in UTF8 string.\n");

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


// Util classes
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
            dal::Texture  tex;
            int32_t       advance = 0;
        };

    private:
        class FreetypeMaster {

        public:
            class FreetypeFace {

            private:
                FT_Face m_face;
                std::vector<FT_Byte> m_fontData;
                std::string m_fontName;

            public:
                FreetypeFace(const FreetypeFace&) = delete;
                FreetypeFace& operator=(const FreetypeFace&) = delete;

            public:
                FreetypeFace(void)
                    : m_face(nullptr)
                {

                }

                FreetypeFace(const dal::ResourceID& fontResID, FT_Library ftLib) {
                    if ( !dal::futil::getRes_buffer(fontResID, this->m_fontData) ) {
                        dalAbort("Failed to load font file: {}"_format(fontResID.makeIDStr()));
                    }

                    const auto error = FT_New_Memory_Face(ftLib, this->m_fontData.data(), static_cast<FT_Long>(this->m_fontData.size()), 0, &this->m_face);
                    if ( error ) {
                        dalAbort("Failed to initiate freetype face: {}"_format(fontResID.makeIDStr()));
                    }

                    this->setPixelSize(17);
                }

                FreetypeFace(FreetypeFace&& other) noexcept
                    : m_face(other.m_face)
                    , m_fontData(std::move(other.m_fontData))
                    , m_fontName(std::move(other.m_fontName))
                {
                    other.m_face = nullptr;
                }

                FreetypeFace& operator=(FreetypeFace&& other) noexcept {
                    this->invalidateFace();

                    this->m_face = other.m_face;
                    other.m_face = nullptr;

                    std::swap(this->m_fontData, other.m_fontData);
                    std::swap(this->m_fontName, other.m_fontName);

                    return *this;
                }

                ~FreetypeFace(void) {
                    this->invalidateFace();
                }

                void setPixelSize(FT_UInt size) {
                    FT_Set_Pixel_Sizes(this->m_face, 0, size);
                }

                void loadCharData(const uint32_t utf32Char, CharacterUnit& charUnit) {
                    const auto glyphIndex = FT_Get_Char_Index(this->m_face, utf32Char);
                    if ( FT_Load_Glyph(this->m_face, glyphIndex, FT_LOAD_RENDER) != 0 ) {
                        dalAbort("Failed to get Glyph for utf-32 char \'{}\' in font \"{}\""_format(utf32Char, this->m_fontName));
                    }

                    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // Text looks broken without this.
                    charUnit.tex.init_maskMap(this->m_face->glyph->bitmap.buffer, this->m_face->glyph->bitmap.width, this->m_face->glyph->bitmap.rows);
                    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

                    charUnit.size = glm::vec2{ this->m_face->glyph->bitmap.width, this->m_face->glyph->bitmap.rows };
                    charUnit.bearing = glm::vec2{ this->m_face->glyph->bitmap_left, this->m_face->glyph->bitmap_top };
                    charUnit.advance = static_cast<int32_t>(this->m_face->glyph->advance.x);
                }

            private:
                void loadChar(const char c) {
                    if ( FT_Load_Char(this->m_face, c, FT_LOAD_RENDER) ) {
                        dalAbort("Failed to load Glyph \'{}\' for font \"{}\""_format(c, this->m_fontName));
                    }
                }

                void invalidateFace(void) {
                    if ( nullptr != this->m_face ) {
                        FT_Done_Face(this->m_face);
                        this->m_face = nullptr;
                    }
                }

            };

        private:
            FT_Library m_library = nullptr;
            std::unordered_map<std::string, FreetypeFace> m_faces;

        public:
            FreetypeMaster(const FreetypeMaster&) = delete;
            FreetypeMaster(FreetypeMaster&&) = delete;
            FreetypeMaster& operator=(const FreetypeMaster&) = delete;
            FreetypeMaster& operator=(FreetypeMaster&&) = delete;

        public:
            FreetypeMaster(void) {
                if ( FT_Init_FreeType(&this->m_library) != 0 ) {
                    dalAbort("Failed to init FreeType library.");
                }
            }

            ~FreetypeMaster(void) {
                this->m_faces.clear();
                FT_Done_FreeType(this->m_library);
            }

            FreetypeFace& getFace(const std::string& fontResBasicForm) {
                auto found = this->m_faces.find(fontResBasicForm);

                if ( this->m_faces.end() != found ) {
                    return found->second;
                }
                else {
                    auto [inserted, success] = this->m_faces.emplace(fontResBasicForm, FreetypeFace{ fontResBasicForm, this->m_library });
                    dalAssert(success);
                    return inserted->second;
                }
            }

        };

        class CharMap {

        private:
            unsigned int m_fontSize;
            std::string m_fontName;
            FreetypeMaster& m_freetype;
            std::unordered_map<uint32_t, CharacterUnit> m_unicodes;
            std::array<CharacterUnit, 256> m_asciis;

        public:
            CharMap(const CharMap&) = delete;
            CharMap& operator=(const CharMap&) = delete;

            CharMap(CharMap&&) noexcept = default;
            CharMap& operator=(CharMap&&) noexcept = default;

        public:
            CharMap(unsigned int fontSize, const std::string& fontName, FreetypeMaster& freetype) 
                : m_fontSize(fontSize)
                , m_fontName(fontName)
                , m_freetype(freetype)
            {
                for ( int i = 0; i < 256; ++i ) {
                    auto& face = freetype.getFace(this->m_fontName);
                    face.setPixelSize(this->m_fontSize);
                    face.loadCharData(i, this->m_asciis[i]);
                }
            }

            const CharacterUnit& operator[](const uint32_t utf32Code) {
                if ( utf32Code < 256 ) {
                    return this->m_asciis[utf32Code];
                }
                else {
                    return this->getUnicode(utf32Code);
                }
            }

        private:
            const CharacterUnit& getUnicode(const uint32_t utf32Code) {
                const auto found = this->m_unicodes.find(utf32Code);
                if ( this->m_unicodes.end() != found ) {
                    return found->second;
                }
                else {
                    auto [iter, success] = this->m_unicodes.emplace(utf32Code, CharacterUnit{});
                    dalAssert(success);

                    auto& face = this->m_freetype.getFace(this->m_fontName);
                    face.setPixelSize(this->m_fontSize);
                    face.loadCharData(utf32Code, iter->second);

                    return iter->second;
                }
            }

        };

    private:
        static inline const std::string BASIC_FONT_NAME{ "asset::NanumGothic.ttf" };
        std::vector<std::unique_ptr<CharMap>> m_charMaps;
        FreetypeMaster m_freetypeMas;

    public:
        const CharacterUnit& at(const uint32_t utf32Char, unsigned int textSize) {
            return this->getCharMap(textSize)[utf32Char];
        }

    private:
        CharMap& getCharMap(const unsigned int fontSize) {
            if ( this->m_charMaps.size() > fontSize ) {
                if ( nullptr == this->m_charMaps[fontSize] ) {
                    this->m_charMaps[fontSize].reset(new CharMap{ fontSize, this->BASIC_FONT_NAME, this->m_freetypeMas });
                }
                return *(this->m_charMaps[fontSize].get());
            }
            else {
                this->m_charMaps.resize(static_cast<size_t>(fontSize) + 1);
                this->m_charMaps[fontSize].reset(new CharMap{ fontSize, this->BASIC_FONT_NAME, this->m_freetypeMas });
                return *(this->m_charMaps[fontSize].get());
            }
        }

    } g_charCache;

}


// Header functions
namespace dal {

    inline glm::vec2 screen2device(const glm::vec2& p, const float winWidth, const float winHeight) {
        return glm::vec2{
             2.0f * p.x / (winWidth) - 1.0f,
            -2.0f * p.y / (winHeight) + 1.0f
        };
    }

    inline glm::vec2 screen2device(const glm::vec2& p, const unsigned int winWidth, const unsigned int winHeight) {
        return screen2device(p, static_cast<float>(winWidth), static_cast<float>(winHeight));
    }

    inline glm::vec2 device2screen(const glm::vec2& p, const float winWidth, const float winHeight) {
        return glm::vec2{
            (p.x + 1.0f) * (winWidth) / 2.0f,
            (1.0f - p.y) * (winHeight) / 2.0f
        };
    }

    inline glm::vec2 device2screen(const glm::vec2& p, const unsigned int winWidth, const unsigned int winHeight) {
        return device2screen(p, static_cast<float>(winWidth), static_cast<float>(winHeight));
    }


    void renderQuadOverlay(const UnilocOverlay& uniloc, const glm::vec2& devSpcP1, const glm::vec2& devSpcP2, const glm::vec4& color,
        const Texture* const diffuseMap, const Texture* const maskMap, const bool upsideDown_diffuseMap, const bool upsideDown_maskMap,
        const glm::vec2& texOffset, const glm::vec2& texScale)
    {
        uniloc.point1(devSpcP1);
        uniloc.point2(devSpcP2);
        uniloc.color(color);
        uniloc.upsideDownDiffuseMap(upsideDown_diffuseMap);
        uniloc.upsideDownMaskMap(upsideDown_maskMap);
        uniloc.texOffset(texOffset);
        uniloc.texScale(texScale);

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

    void renderQuadOverlay(const UnilocOverlay& uniloc, const glm::vec2& devSpcP1, const glm::vec2& devSpcP2, const glm::vec4& color,
        const Texture* const diffuseMap, const Texture* const maskMap, const bool upsideDown_diffuseMap, const bool upsideDown_maskMap)
    {
        renderQuadOverlay(uniloc, devSpcP1, devSpcP2, color, diffuseMap, maskMap,
			upsideDown_diffuseMap, upsideDown_maskMap, glm::vec2{ 0.f }, glm::vec2{ 1.f });
    }

    void renderQuadOverlay(const UnilocOverlay& uniloc, const std::pair<glm::vec2, glm::vec2>& devSpc, const glm::vec4& color,
        const Texture* const diffuseMap, const Texture* const maskMap, const bool upsideDown_diffuseMap, const bool upsideDown_maskMap)
    {
        renderQuadOverlay(uniloc, devSpc.first, devSpc.second, color, diffuseMap, maskMap, upsideDown_diffuseMap, upsideDown_maskMap);
    }

    void renderQuadOverlay(const UnilocOverlay& uniloc, const glm::vec2& devSpcP1, const glm::vec2& devSpcP2, const glm::vec4& color) {
        renderQuadOverlay(uniloc, devSpcP1, devSpcP2, color, nullptr, nullptr, false, false);
    }

    void renderQuadOverlay(const UnilocOverlay& uniloc, const QuadRenderInfo& info) {
        renderQuadOverlay(uniloc, info.m_devSpcP1, info.m_devSpcP2, info.m_color, info.m_diffuseMap, info.m_maskMap,
            info.m_upsideDown_diffuse, info.m_upsideDown_mask, info.m_texOffset, info.m_texScale);
    }

}


// ScreenSpaceBox
namespace dal {

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
        , m_textColor(1.0f, 1.0f, 1.0f, 1.0f)
        , m_cursorPos(cursorNullPos)
        , m_textSize(15)
        , m_lineSpacingRate(1.3f)
        , m_wordWrap(false)
        , m_owning(-1)
    {

    }

    void TextRenderer::render(const UnilocOverlay& uniloc, const float width, const float height) {
        this->makeOffsetApproch();

        const auto parentP2 = this->getPoint11();

        const float xInit = this->m_wordWrap ? this->getPosX() : (this->getPosX() + this->m_offset.x);
        float xAdvance = xInit;
        float yHeight = this->getPosY() + static_cast<float>(this->m_textSize) + this->m_offset.y;

        auto header = this->m_text.c_str();
        const auto end = header + this->m_text.size();

        int64_t charCount = -1;

        while ( end != header ) {
            ++charCount;

            uint32_t c = 0;

            // Fetch utf-32 char
            {
                const auto ch = static_cast<uint8_t>(*header);
                const auto codeSize = utf8_codepoint_size(ch);
                dalAssert(codeSize <= MAX_UTF8_CODE_SIZE);

                if ( codeSize > 1 ) {
                    c = convertUTF8to32(reinterpret_cast<const uint8_t*>(header));
                    header += codeSize;
                }
                else {
                    c = ch; ++header;
                }
            }

            // Process control char
            {
                if ( ' ' == c ) {
                    xAdvance += g_charCache.at(c, this->m_textSize).advance >> 6;
                    continue;
                }
                else if ( '\n' == c ) {
                    yHeight += static_cast<float>(this->m_textSize) * this->m_lineSpacingRate;
                    xAdvance = xInit;
                    continue;
                }
                else if ( '\t' == c ) {
                    xAdvance += TAP_CHAR_WIDTH;
                    continue;
                }
            }

            auto& charInfo = g_charCache.at(c, this->m_textSize);

            // Carriage return if current line is full.
            if ( xAdvance >= parentP2.x ) {
                if ( this->m_wordWrap ) {
                    yHeight += static_cast<float>(this->m_textSize) * this->m_lineSpacingRate;
                    xAdvance = xInit;
                }
                else {
                    header = findNextChar<'\n'>(header, end);
                    continue;
                }
            }

            std::pair<glm::vec2, glm::vec2> charQuad;

            charQuad.first.x = roundf(xAdvance + charInfo.bearing.x);
            charQuad.first.y = roundf(yHeight - charInfo.bearing.y);
            charQuad.second.x = charQuad.first.x + static_cast<float>(charInfo.size.x);
            charQuad.second.y = charQuad.first.y + static_cast<float>(charInfo.size.y);

            const auto flag = this->isCharQuadInside(charQuad.first, charQuad.second);
            if ( CharPassFlag::breakk == flag ) {
                break;
            }
            else if ( CharPassFlag::continuee == flag ) {
                xAdvance += (charInfo.advance >> 6);
                continue;
            }
            else if ( CharPassFlag::carriageReturn == flag ) {
                header = findNextChar<'\n'>(header, end);
                continue;
            }

            if ( this->m_cursorPos == charCount && this->canDrawCursor() ) {
                const auto p1 = glm::vec2{ charQuad.second.x, charQuad.second.y - static_cast<float>(this->m_textSize) };
                const auto p2 = glm::vec2{ charQuad.second.x + 1.0f, charQuad.second.y };
                const auto cursorPos1 = screen2device(p1, width, height);
                const auto cursorPos2 = screen2device(p2, width, height);
                renderQuadOverlay(uniloc, cursorPos1, cursorPos2, this->m_textColor);
            }

            const auto charQuadCut = this->makeCutCharArea(charQuad.first, charQuad.second);

            QuadRenderInfo charQuadInfo;
            {
                charQuadInfo.m_devSpcP1 = screen2device(charQuadCut.first, width, height);
                charQuadInfo.m_devSpcP2 = screen2device(charQuadCut.second, width, height);

                charQuadInfo.m_color = this->m_textColor;
                charQuadInfo.m_maskMap = &charInfo.tex;

                if ( charQuadCut.first != charQuad.first || charQuadCut.second != charQuad.second ) {
                    const auto [scale, offset] = calcScaleOffset(charQuad, charQuadCut);

                    charQuadInfo.m_texScale = scale;
                    charQuadInfo.m_texOffset = offset;
                }
            }

            renderQuadOverlay(uniloc, charQuadInfo);

            xAdvance += (charInfo.advance >> 6);
        }

        if ( this->m_cursorPos == charCount && this->canDrawCursor() ) {
            const auto p1 = glm::vec2{ xAdvance, yHeight - static_cast<float>(this->m_textSize) };
            const auto p2 = glm::vec2{ xAdvance + 1.0f, yHeight };
            const auto cursorPos1 = screen2device(p1, width, height);
            const auto cursorPos2 = screen2device(p2, width, height);
            renderQuadOverlay(uniloc, cursorPos1, cursorPos2, this->m_textColor);
        }
    }

    InputCtrlFlag TextRenderer::onTouch(const TouchEvent& e) {
        if ( -1 != this->m_owning ) {
            if ( e.m_id == this->m_owning ) {
                if ( TouchActionType::move == e.m_actionType ) {
                    const auto rel = e.m_pos - this->m_lastTouchPos;
                    this->m_lastTouchPos = e.m_pos;
                    this->m_offset += rel;
                    return InputCtrlFlag::owned;
                }
                else if ( TouchActionType::up == e.m_actionType ) {
                    const auto rel = e.m_pos - this->m_lastTouchPos;
                    this->m_lastTouchPos = e.m_pos;
                    this->m_offset += rel;

                    this->m_owning = -1;
                    return InputCtrlFlag::consumed;
                }
                // If else ignores.
            }
            // If else ignores.
        }
        else {
            if ( TouchActionType::down == e.m_actionType ) {
                this->m_lastTouchPos = e.m_pos;
                this->m_owning = e.m_id;
                return InputCtrlFlag::owned;
            }
            // If else ignores.
        }

        return InputCtrlFlag::ignored;
    }

    // Private

    bool TextRenderer::canDrawCursor(void) {
        return std::fmodf(this->m_cursorTimer.getElapsed(), 1.0f) >= 0.5f;
    }

    TextRenderer::CharPassFlag TextRenderer::isCharQuadInside(glm::vec2& p1, glm::vec2& p2) const {
        const auto pp11 = this->getPoint11();

        if ( p1.x > pp11.x ) {
            // This shouldn't happen because carriage return already dealt in render function body.
            return CharPassFlag::carriageReturn;
        }
        else if ( p1.y > pp11.y ) {
            return CharPassFlag::continuee;
        }
        else if ( p2.x < this->getPosX() ) {
            return CharPassFlag::continuee;
        }
        else if ( p2.y < this->getPosY() ) {
            return CharPassFlag::continuee;
        }
        else {
            return CharPassFlag::okk;
        }
    }

    void TextRenderer::makeOffsetApproch(void) {
        if ( -1 != this->m_owning ) {
            return;
        }

        if ( this->m_offset.x > 0 ) {
            this->m_offset.x = this->m_offset.x * 0.9f;
        }
        if ( this->m_offset.y > 0 ) {
            this->m_offset.y = this->m_offset.y * 0.9f;
        }
    }

    std::pair<glm::vec2, glm::vec2> TextRenderer::makeCutCharArea(glm::vec2 p1, glm::vec2 p2) {
        if ( p1.x < this->getPosX() ) {
            p1.x = this->getPosX();
        }
        if ( p1.y < this->getPosY() ) {
            p1.y = this->getPosY();
        }

        const auto pp11 = this->getPoint11();

        if ( p2.x > pp11.x ) {
            p2.x = pp11.x;
        }
        if ( p2.y > pp11.y ) {
            p2.y = pp11.y;
        }

        return std::make_pair(p1, p2);
    }

}
