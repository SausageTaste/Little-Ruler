#pragma once

#include <vector>
#include <string>

extern "C" {
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
}


namespace dal {

	class FreetypeGod {

	private:
		FT_Library mFLibrary;
		FT_Face mFace;
		
	/////////////////////////////////

		FreetypeGod(void);
		~FreetypeGod(void);
		FreetypeGod(FreetypeGod&) = delete;
		FreetypeGod& operator=(FreetypeGod&) = delete;

	public:
		static FreetypeGod& getinst(void);
		FT_Face& getFace(void);
		void setFontSize(unsigned int v);

	};
}