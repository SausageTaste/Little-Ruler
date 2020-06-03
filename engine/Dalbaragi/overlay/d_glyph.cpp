#include "d_glyph.h"

#include <array>
#include <optional>
#include <unordered_map>

extern "C" {
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
}


namespace {

    class FreetypeFace {

    private:
        FT_Face m_face = nullptr;
        std::vector<FT_Byte> m_fontData;
        std::string m_fontName;

    public:
        FreetypeFace(const FreetypeFace&) = delete;
        FreetypeFace& operator=(const FreetypeFace&) = delete;

    public:
        FreetypeFace(void) = default;
        FreetypeFace(const char* const fontRespath, const FT_Library ftLib, dal::loadFileFunc_t loadfunc) {
            if ( !loadfunc(fontRespath, this->m_fontData) ) {
                assert(false && "failed to load font file");
            }

            const auto error = FT_New_Memory_Face(ftLib, this->m_fontData.data(), static_cast<FT_Long>(this->m_fontData.size()), 0, &this->m_face);
            assert(!error);

            FT_Select_Charmap(this->m_face, ft_encoding_unicode);

            this->setPixelSize(17);
        }
        ~FreetypeFace(void) {
            this->invalidate();
        }

        FreetypeFace(FreetypeFace&& other) noexcept
            : m_face(other.m_face)
            , m_fontData(std::move(other.m_fontData))
            , m_fontName(std::move(other.m_fontName))
        {
            other.m_face = nullptr;
        }
        auto operator=(FreetypeFace&& other) noexcept -> FreetypeFace& {
            this->invalidate();

            this->m_face = other.m_face;
            other.m_face = nullptr;

            std::swap(this->m_fontData, other.m_fontData);
            std::swap(this->m_fontName, other.m_fontName);

            return *this;
        }

        void setPixelSize(const FT_UInt size) {
            FT_Set_Pixel_Sizes(this->m_face, 0, size);
        }

        void loadCharData(const dal::utf32_t c, dal::CharUnit& charUnit) const {
            const auto glyphIndex = FT_Get_Char_Index(this->m_face, c);
            //assert(0 != glyphIndex);

            const auto loadResult = FT_Load_Glyph(this->m_face, glyphIndex, FT_LOAD_RENDER);
            assert(0 == loadResult);

            assert(nullptr != charUnit.m_tex);
            charUnit.m_tex->init_maskMap(this->m_face->glyph->bitmap.buffer, this->m_face->glyph->bitmap.width, this->m_face->glyph->bitmap.rows);

            charUnit.m_size.x = this->m_face->glyph->bitmap.width;
            charUnit.m_size.y = this->m_face->glyph->bitmap.rows;

            charUnit.m_bearing.x = this->m_face->glyph->bitmap_left;
            charUnit.m_bearing.y = this->m_face->glyph->bitmap_top;

            charUnit.m_advance = static_cast<int32_t>(this->m_face->glyph->advance.x);
        }

    private:
        bool loadChar(const char c) {
            return 0 != FT_Load_Char(this->m_face, c, FT_LOAD_RENDER);
        }

        void invalidate(void) {
            if ( nullptr != this->m_face ) {
                FT_Done_Face(this->m_face);
                this->m_face = nullptr;
            }
        }

    };

}


// FreetypeMaster
namespace dal {

    class GlyphMaster::FreetypeMaster {

    private:
        FT_Library m_library = nullptr;
        std::unordered_map<std::string, FreetypeFace> m_faces;

        loadFileFunc_t m_loadfunc;
        texGenFunc_t m_texfunc;

    public:
        FreetypeMaster(loadFileFunc_t loadfunc, texGenFunc_t texfunc)
            : m_loadfunc(loadfunc)
            , m_texfunc(texfunc)
        {
            if ( 0 != FT_Init_FreeType(&this->m_library) ) {
                assert(false && "failed to init FreeType library");
            }
        }
        ~FreetypeMaster(void) {
            this->m_faces.clear();
            FT_Done_FreeType(this->m_library);
            this->m_library = nullptr;
        }

        auto getFace(const std::string& fontRespath) -> FreetypeFace& {
            auto found = this->m_faces.find(fontRespath);

            if ( this->m_faces.end() != found ) {
                return found->second;
            }
            else {
                auto [inserted, success] = this->m_faces.emplace(fontRespath, FreetypeFace{ fontRespath.c_str(), this->m_library, this->m_loadfunc });
                assert(success);
                return inserted->second;
            }
        }

        auto genTexture(void) const -> OverlayTexture* {
            return this->m_texfunc();
        }

    };

}


// GlyphMap
namespace dal {

    class GlyphMaster::GlyphMap {

    private:
        std::array<CharUnit, 256> m_asciis;
        std::unordered_map<utf32_t, CharUnit> m_unicodes;
        std::string m_fontName;
        unsigned m_fontSize;

        FreetypeMaster* m_freetype;

    public:
        GlyphMap(const GlyphMap&) = delete;
        GlyphMap& operator=(const GlyphMap&) = delete;

    public:
        GlyphMap(const std::string& fontRespath, const unsigned fontSize, FreetypeMaster& ft)
            : m_fontName(fontRespath)
            , m_fontSize(fontSize)
            , m_freetype(&ft)
        {
            for ( int i = 0; i < 256; ++i ) {
                auto& face = ft.getFace(this->m_fontName);
                face.setPixelSize(this->m_fontSize);
                this->m_asciis[i].m_tex.reset(ft.genTexture());
                face.loadCharData(i, this->m_asciis[i]);
            }
        }

        GlyphMap(GlyphMap&& other) noexcept {
            this->m_asciis = std::move(other.m_asciis);
            this->m_unicodes = std::move(other.m_unicodes);
            this->m_fontName = std::move(other.m_fontName);
            this->m_fontSize = other.m_fontSize;
            this->m_freetype = other.m_freetype;
        }
        GlyphMap& operator=(GlyphMap&& other) noexcept {
            this->m_asciis = std::move(other.m_asciis);
            this->m_unicodes = std::move(other.m_unicodes);
            this->m_fontName = std::move(other.m_fontName);
            this->m_fontSize = other.m_fontSize;
            this->m_freetype = other.m_freetype;

            return *this;
        }

        auto get(const utf32_t c) -> const CharUnit& {
            if ( c < 256 ) {
                return this->m_asciis[c];
            }
            else {
                return this->getUnicode(c);
            }
        }
        auto fontSize(void) const {
            return this->m_fontSize;
        }

    private:
        auto getUnicode(const utf32_t c) -> const CharUnit& {
            const auto found = this->m_unicodes.find(c);

            if ( found != this->m_unicodes.end() ) {
                return found->second;
            }
            else {
                auto [iter, success] = this->m_unicodes.emplace(c, CharUnit{});
                assert(success);
                assert(nullptr != this->m_freetype);

                auto& unit = iter->second;
                unit.m_tex.reset(this->m_freetype->genTexture());

                auto& face = this->m_freetype->getFace(this->m_fontName);
                face.setPixelSize(this->m_fontSize);
                face.loadCharData(c, unit);

                return unit;
            }
        }

    };

}


namespace dal {

    class GlyphMaster::FontSizesMap {

    private:
        std::string m_fontRespath;
        std::vector<GlyphMaster::GlyphMap> m_maps;

        FreetypeMaster* m_freetype;

    public:
        FontSizesMap(const FontSizesMap&) = delete;
        auto operator=(const FontSizesMap&) -> FontSizesMap& = delete;
        FontSizesMap(FontSizesMap&&) noexcept = default;
        auto operator=(FontSizesMap&&) noexcept -> FontSizesMap& = default;

    public:
        FontSizesMap(const std::string& name, FreetypeMaster& ft)
            : m_fontRespath(name)
            , m_freetype(&ft)
        {
        
        }

        auto getMapOfSizeOf(const unsigned fontSize) -> GlyphMap& {
            for ( auto& map : this->m_maps ) {
                if ( fontSize == map.fontSize() ) {
                    return map;
                }
            }

            return this->m_maps.emplace_back(this->m_fontRespath, fontSize, *this->m_freetype);
        }

    };

}


namespace dal {

    GlyphMaster::GlyphMaster(loadFileFunc_t loadfunc, texGenFunc_t texfunc)
        : m_freetype(new FreetypeMaster{ loadfunc, texfunc })
    {
        const std::array<std::string, 1> fonts = {
            "asset::NotoSansCJKkr-Regular.otf"
        };

        for ( auto& name : fonts ) {
            this->m_mapLists.emplace_back(name, *this->m_freetype);
        }
    }

    GlyphMaster::~GlyphMaster(void) {
        delete this->m_freetype;
        this->m_freetype = nullptr;
    }

    auto GlyphMaster::get(const utf32_t c, unsigned int fontSize) -> const CharUnit& {
        for ( auto& maps : this->m_mapLists ) {
            auto& map = maps.getMapOfSizeOf(fontSize);
            return map.get(c);
        }

        assert(false);
        static CharUnit empty{};
        return empty;
    }

}
