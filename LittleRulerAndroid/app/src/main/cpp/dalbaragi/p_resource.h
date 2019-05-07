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
	class Package;


	class ModelHandle {

	private:
		struct Pimpl;
		Pimpl* pimpl;

	public:
		ModelHandle(void);
		ModelHandle(const char* const modelID, Model* const model);
		ModelHandle(const ModelHandle& other);
		ModelHandle(ModelHandle&& other) noexcept;
		ModelHandle& operator=(const ModelHandle& other);
		ModelHandle& operator=(ModelHandle&& other) noexcept;
		~ModelHandle(void);

		void renderGeneral(const UnilocGeneral& uniloc, const std::list<Actor>& actors) const;

	};


	class Package {

	private:
		std::string m_name;
		std::unordered_map<std::string, ModelHandle> m_models;

	public:
		void setName(const char* const packageName);
		void setName(const std::string& packageName);

		ModelHandle orderModel(const ResourceFilePath& resPath);
		ModelHandle buildModel(const buildinfo::ModelDefined& info);

	};\


	class ResourceMaster {

		//////// Attribs ////////

	private:
		std::unordered_map<std::string, Package> m_packages;

		//////// Methods ////////

	public:
		ModelHandle orderModel(const char* const packageName_dir_modelID);

		ModelHandle buildModel(const buildinfo::ModelDefined& info, const char* const packageName);

	private:
		Package& orderPackage(const std::string& packName);

	};

}