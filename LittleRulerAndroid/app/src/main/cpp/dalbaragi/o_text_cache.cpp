#include "o_text_cache.h"

#include <string>

#include "s_logger_god.h"
#include "s_freetype_master.h"


using namespace std::string_literals;


namespace {

	auto g_logger = dal::LoggerGod::getinst();

}


namespace dal {

	UnicodeCache::UnicodeCache(ResourceMaster& resMas)
	:	m_resMas(resMas)
	{

	}

	const CharacterUnit& UnicodeCache::at(const uint32_t utf32Char) {
		const auto found = this->m_cache.find(utf32Char);

		if (this->m_cache.end() == found) {
			auto added = this->m_cache.emplace(utf32Char, CharacterUnit{});
			if (false == added.second) {
				g_logger.putFatal("Failed to add a glyph in dal::UnicodeCache::at");
				throw - 1;
			}
			auto& charUnit = added.first->second;

			auto& face = dal::FreetypeGod::getinst().getFace();
			const auto glyphIndex = FT_Get_Char_Index(face, utf32Char);
			if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_RENDER) != 0) {
				g_logger.putFatal("Failed to load Glyph: "s + std::to_string(utf32Char));
				throw - 1;
			}

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // Text looks broken without this.
			charUnit.tex = ResourceMaster::getMaskMap(face->glyph->bitmap.buffer, face->glyph->bitmap.width, face->glyph->bitmap.rows);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 0);

			charUnit.size = glm::vec2{ face->glyph->bitmap.width, face->glyph->bitmap.rows };
			charUnit.bearing = glm::vec2{ face->glyph->bitmap_left, face->glyph->bitmap_top };
			charUnit.advance = static_cast<int32_t>(face->glyph->advance.x);

			g_logger.putInfo("Unicode glyph created: "s + std::to_string(utf32Char));

			return charUnit;
		}
		else {
			return found->second;
		}
	}

}