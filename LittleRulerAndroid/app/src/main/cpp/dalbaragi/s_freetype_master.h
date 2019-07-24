#pragma once

extern "C" {
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
}


namespace dal {

    class FreetypeGod {

        //////// Var ////////

    private:
        FT_Library mFLibrary;
        FT_Face mFace;

        //////// Func ////////

        FreetypeGod(void);
        ~FreetypeGod(void);

    public:
        FreetypeGod(const FreetypeGod&) = delete;
        FreetypeGod(FreetypeGod&&) = delete;
        FreetypeGod& operator=(const FreetypeGod&) = delete;
        FreetypeGod& operator=(FreetypeGod&&) = delete;

    public:
        static FreetypeGod& getinst(void);
        FT_Face& getFace(void);
        void setFontSize(unsigned int v);

    };

}