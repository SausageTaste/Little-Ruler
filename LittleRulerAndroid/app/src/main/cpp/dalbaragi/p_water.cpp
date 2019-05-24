#include "p_water.h"

#include <array>

#include "s_logger_god.h"


namespace dal {

	WaterRenderer::WaterRenderer(const glm::vec3& pos, const glm::vec2& size) {
		std::array<float, 18> vertices{
			pos.x,          pos.y, pos.z,
			pos.x,          pos.y, pos.z + size.y,
			pos.x + size.x, pos.y, pos.z + size.y,
			pos.x,          pos.y, pos.z,
			pos.x + size.x, pos.y, pos.z + size.y,
			pos.x + size.x, pos.y, pos.z
		};
		std::array<float, 12> texcoords{
			0, 1,
			0, 0,
			1, 0,
			0, 1,
			1, 0,
			1, 1
		};
		std::array<float, 18> normals{
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0
		};

		this->m_mesh.buildData(
			vertices.data(), vertices.size(),
			texcoords.data(), texcoords.size(),
			normals.data(), normals.size()
		);

		this->m_material.m_diffuseColor = { 0.0f, 0.0f, 1.0 };
	}

	void WaterRenderer::renderWaterry(const UnilocWaterry& uniloc) {
		this->m_material.sendUniform(uniloc);
		const glm::mat4 mat{ 1.0f };
		glUniformMatrix4fv(uniloc.uModelMat, 1, GL_FALSE, &mat[0][0]);
		this->m_mesh.draw();
	}

}