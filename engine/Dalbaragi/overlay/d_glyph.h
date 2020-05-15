#pragma once

#include <vector>
#include <memory>
#include <string>
#include <functional>

#include "d_overlay_base.h"


namespace dal {

    using utf32_t = uint32_t;

    using loadFileFunc_t = std::function<bool(const char*, std::vector<uint8_t>&)>;
    using texGenFunc_t = std::function<OverlayTexture* (void)>;


    struct CharUnit {
        glm::ivec2 m_size{ 0 };
        glm::ivec2 m_bearing{ 0 };
        std::int32_t m_advance = 0;
        std::unique_ptr<dal::OverlayTexture> m_tex;
    };


    class GlyphMaster {

    private:
        class FreetypeMaster;
        class GlyphMap;
        class FontSizesMap;

    private:
        std::vector<FontSizesMap> m_mapLists;
        std::unique_ptr<FreetypeMaster> m_freetype;

    public:
        GlyphMaster(const GlyphMaster&) = delete;
        GlyphMaster& operator=(const GlyphMaster&) = delete;

    public:
        GlyphMaster(loadFileFunc_t loadfunc, texGenFunc_t texfunc);

        auto get(const utf32_t c, unsigned int fontSize) -> const CharUnit&;

    };

}
