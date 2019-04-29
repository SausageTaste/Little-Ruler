#pragma once

#include <glm/glm.hpp>

#include "p_texture.h"


namespace dal {

	struct CharacterUnit {
		TextureHandle_ptr tex;
		glm::ivec2        size;
		glm::ivec2        bearing;
		int32_t           advance = 0;
	};

	class CharMaskMapCache {

	private:
		CharacterUnit mAsciiChars[128];

	public:
		CharMaskMapCache(TextureMaster& texMaster);
		const CharacterUnit& at(unsigned int index) const;

	};

}
