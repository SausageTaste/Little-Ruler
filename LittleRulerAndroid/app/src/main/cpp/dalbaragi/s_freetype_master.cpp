#include "s_freetype_master.h"

#include <string>

#include "s_logger_god.h"
#include "u_fileclass.h"


using namespace std::string_literals;


namespace dal {

	FreetypeGod::FreetypeGod(void) {
		if (FT_Init_FreeType(&mFLibrary) != 0) {
			dalAbort("Failed to init FreeType library.");
		}

		const char* const fontName = "asset::NanumGothic.ttf";
		auto buffer = new std::vector<FT_Byte>;  // Freetype needs this not deleted.
		if ( !futil::getRes_buffer(fontName, *buffer) )
			dalAbort("Failed to load font file: "s + fontName);

		const auto error = FT_New_Memory_Face(mFLibrary, buffer->data(), static_cast<FT_Long>(buffer->size()), 0, &mFace);
		if ( error )
			dalAbort("Failed to find font: "s + fontName);

		FT_Set_Pixel_Sizes(mFace, 0, 17);

		if (FT_Load_Char(mFace, 'X', FT_LOAD_RENDER)) {
			dalAbort("Failed to load Glyph.");
		}
	}

	FreetypeGod::~FreetypeGod(void) {
		FT_Done_Face(mFace);
		FT_Done_FreeType(mFLibrary);
	}

	FreetypeGod& FreetypeGod::getinst(void) {
		static FreetypeGod inst;
		return inst;
	}

	FT_Face& FreetypeGod::getFace(void) {
		return mFace;
	}

	void FreetypeGod::setFontSize(unsigned int v) {
		FT_Set_Pixel_Sizes(mFace, 0, v);
	}

}