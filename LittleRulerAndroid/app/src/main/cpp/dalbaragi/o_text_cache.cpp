#include "o_text_cache.h"

#include <string>

#include "s_logger_god.h"
#include "s_freetype_master.h"


using namespace std;


namespace dal {

	CharMaskMapCache::CharMaskMapCache(TextureMaster& texMaster) {
		auto& face = dal::FreetypeGod::getinst().getFace();
		auto& logger = dal::LoggerGod::getinst();

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		for (int i = 0; i < 128; i++) {
			if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
				logger.putError("Failed to load Glyph: "s + to_string(char(i)));
				continue;
			}

			auto& charac = mAsciiChars[i];
			charac.tex = texMaster.request_maskMap(
				face->glyph->bitmap.buffer, face->glyph->bitmap.width, face->glyph->bitmap.rows
			);

			charac.size = { face->glyph->bitmap.width, face->glyph->bitmap.rows };
			charac.bearing = { face->glyph->bitmap_left, face->glyph->bitmap_top };
			charac.advance = face->glyph->advance.x;
		}
	}

	const CharacterUnit& CharMaskMapCache::at(unsigned int index) const {
		if (index < 128) {
			return mAsciiChars[index];
		}
		else {
			return mAsciiChars[(int)'?'];
		}
	}

}
