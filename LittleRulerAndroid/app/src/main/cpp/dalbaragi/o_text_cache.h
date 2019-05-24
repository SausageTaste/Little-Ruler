#pragma once

#include <unordered_map>

#include <glm/glm.hpp>

#include "p_resource.h"


namespace dal {

	struct CharacterUnit {
		Texture*   tex = nullptr;
		glm::ivec2 size;
		glm::ivec2 bearing;
		int32_t    advance = 0;
	};


	class UnicodeCache {

	private:
		std::unordered_map<uint32_t, CharacterUnit> m_cache;
		ResourceMaster& m_resMas;

	public:
		UnicodeCache(ResourceMaster& resMas);
		const CharacterUnit& at(const uint32_t index);

	};

}
