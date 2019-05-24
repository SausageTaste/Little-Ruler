#pragma once

#include <glm/glm.hpp>

#include "p_meshStatic.h"
#include "p_dalopengl.h"
#include "p_uniloc.h"
#include "p_resource.h"


namespace dal {
	
	class WaterRenderer {
	
	private:
		MeshStatic m_mesh;
		Material m_material;

	public:
		WaterRenderer(const glm::vec3& pos, const glm::vec2& size);
		void renderWaterry(const UnilocWaterry& uniloc);
	
	};

}