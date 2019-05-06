#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "u_loadinfo.h"
#include "p_material.h"
#include "p_meshStatic.h"
#include "p_texture.h"


namespace dal {

	class ResourceMaster {

		//////// Definitions ////////

	private:
		struct RenderUnit {
			MeshStatic m_mesh;
			Material m_material;
		};

		struct Model {
			const std::string m_id;
			std::vector<RenderUnit> m_renderUnits;
		};

		struct Package {
			const std::string m_name;
			std::unordered_map<std::string, Model> m_models;
			std::unordered_map<std::string, Texture> m_textures;
		};

		//////// Attribs ////////

		std::unordered_map<std::string, Package> m_packages;

		//////// Methods ////////

	public:

	};

}