#include "p_resource.h"

#include "s_logger_god.h"
#include "u_fileclass.h"


using namespace std::string_literals;


namespace {

	struct RenderUnit {
		dal::MeshStatic m_mesh;
		dal::Material m_material;
	};

}

namespace dal {

	struct Model {
		const std::string m_id;
		std::vector<RenderUnit> m_renderUnits;
	};

}


namespace {

	auto& g_logger = dal::LoggerGod::getinst();

	dal::StaticPool<dal::Model, 100> g_modelPool;

}


namespace dal {

	struct ResourceMaster::ModelHandle::Pimpl {
		Model* m_model = nullptr;
		unsigned int m_refCount = 0;
	};

	ResourceMaster::ModelHandle::ModelHandle(Model* model) {
		this->pimpl = new Pimpl();
		this->pimpl->m_refCount = 1;
		this->pimpl->m_model = model;
	}

	ResourceMaster::ModelHandle::ModelHandle(const ResourceMaster::ModelHandle& other) {
		this->pimpl = other.pimpl;
		this->pimpl->m_refCount++;
	}

	ResourceMaster::ModelHandle::ModelHandle(ResourceMaster::ModelHandle&& other) noexcept {
		this->pimpl = other.pimpl;
		other.pimpl = nullptr;
	}

	ResourceMaster::ModelHandle& ResourceMaster::ModelHandle::operator=(const ResourceMaster::ModelHandle& other) {
		this->pimpl = other.pimpl;
		this->pimpl->m_refCount++;

		return *this;
	}

	ResourceMaster::ModelHandle& ResourceMaster::ModelHandle::operator=(ResourceMaster::ModelHandle&& other) noexcept {
		this->pimpl = other.pimpl;
		other.pimpl = nullptr;

		return *this;
	}

	ResourceMaster::ModelHandle::~ModelHandle(void) {
		if (nullptr == this->pimpl) return;

		this->pimpl->m_refCount--;

		if (this->pimpl->m_refCount <= 0) {
			g_logger.putWarn("A model handler's refference all lost without being destroyed: "s + this->pimpl->m_model->m_id);
			delete this->pimpl;
			this->pimpl = nullptr;
		}
	}

}


// Package
namespace dal {

	void ResourceMaster::Package::setName(const char* const packageName) {
		this->m_name = packageName;
	}

	void ResourceMaster::Package::setName(const std::string& packageName) {
		this->m_name = packageName;
	}

	ResourceMaster::ModelHandle ResourceMaster::Package::orderModel(const char* const modelID) {
		
		std::string modelIDStr{ modelID };

		decltype(this->m_models.end()) iter = this->m_models.find(modelIDStr);
		if (this->m_models.end() != iter) {
			return iter->second;
		}
		else {
			g_logger.putTrace("Load model: "s + modelID);
			ResourceMaster::ModelHandle a{ nullptr };
			return a;
		}
		

		//ResourceMaster::ModelHandle a{ nullptr };
		//return a;
	}

}


// ResourceMaster
namespace dal {

	ResourceMaster::ModelHandle ResourceMaster::orderModel(const char* const packageName_modelID) {
		ResourceFilePath path;
		parseResFilePath(packageName_modelID, path);

		auto package = this->orderPackage(path.m_package);

		auto modelName = path.m_name + path.m_ext;
		return package->orderModel(modelName.c_str());
	}

	ResourceMaster::Package* ResourceMaster::orderPackage(const std::string& packName) {
		std::string packNameStr{ packName };
		
		decltype(this->m_packages.end()) iter = this->m_packages.find(packNameStr);
		if (iter != this->m_packages.end()) {
			return &iter->second;
		}
		else { // If not found
			ResourceMaster::Package package;
			package.setName(packName);
			auto res = this->m_packages.emplace(packName, package);
			return &res.first->second;
		}
		
		return nullptr;
	}

}