#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "u_loadinfo.h"
#include "p_material.h"
#include "p_meshStatic.h"
#include "p_texture.h"
#include "u_pool.h"


namespace dal {

	struct Model;


	class ResourceMaster {

		//////// Definitions ////////

	public:
		class ModelHandle {

		private:
			struct Pimpl;
			Pimpl* pimpl;

		public:
			ModelHandle(Model* model);
			ModelHandle(const ModelHandle& other);
			ModelHandle(ModelHandle&& other) noexcept;
			ModelHandle& operator=(const ModelHandle& other);
			ModelHandle& operator=(ModelHandle&& other) noexcept;
			~ModelHandle(void);

		};

		class TextureHandle_V2 {



		};

	private:
		class Package {

		private:
			std::string m_name;
			std::unordered_map<std::string, ModelHandle> m_models;
			std::unordered_map<std::string, TextureHandle_V2> m_textures;

		public:
			void setName(const char* const packageName);
			void setName(const std::string& packageName);
			ModelHandle orderModel(const char* const modelID);

		};

		//////// Attribs ////////

	private:
		std::unordered_map<std::string, Package> m_packages;

		//////// Methods ////////

	public:
		ModelHandle orderModel(const char* const modelID);

	private:
		Package* orderPackage(const std::string& packName);

	};

}