#pragma once

#include <glm/glm.hpp>

#include "p_texture.h"
#include "p_resource.h"


namespace dal {

	struct CharacterUnit {
		TextureHandle2 tex;
		glm::ivec2     size;
		glm::ivec2     bearing;
		int32_t        advance = 0;
	};

	class CharMaskMapCache {

	private:
		CharacterUnit mAsciiChars[128];

	public:
		CharMaskMapCache(TextureMaster& texMaster, ResourceMaster& resMas);
		const CharacterUnit& at(unsigned int index) const;

	};

}
