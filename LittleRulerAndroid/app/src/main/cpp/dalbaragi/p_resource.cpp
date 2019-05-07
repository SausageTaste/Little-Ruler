#include "p_resource.h"

#include "s_logger_god.h"
#include "u_fileclass.h"
#include "s_threader.h"


using namespace std::string_literals;


namespace {

	struct RenderUnit {
		dal::MeshStatic m_mesh;
		dal::Material m_material;
	};

}


namespace {

	class LoadTask_Texture : public dal::ITask {

	private:

	};

}


namespace dal {

	struct Model {
		std::vector<RenderUnit> m_renderUnits;
	};

}


namespace {

	auto& g_logger = dal::LoggerGod::getinst();

	dal::StaticPool<dal::Model, 100> g_modelPool;

}


namespace dal {

	struct ModelHandle::Pimpl {
		std::string m_id;
		Model* m_model = nullptr;
		unsigned int m_refCount = 1;
	};


	ModelHandle::ModelHandle(void) : pimpl(new Pimpl) {

	}

	ModelHandle::ModelHandle(const char* const modelID, Model* const model) : pimpl(new Pimpl) {
		this->pimpl->m_model = model;
	}

	ModelHandle::ModelHandle(const ModelHandle& other) {
		this->pimpl = other.pimpl;
		this->pimpl->m_refCount++;
	}

	ModelHandle::ModelHandle(ModelHandle&& other) noexcept {
		this->pimpl = other.pimpl;
		other.pimpl = nullptr;
	}

	ModelHandle& ModelHandle::operator=(const ModelHandle& other) {
		this->pimpl = other.pimpl;
		this->pimpl->m_refCount++;

		return *this;
	}

	ModelHandle& ModelHandle::operator=(ModelHandle&& other) noexcept {
		this->pimpl = other.pimpl;
		other.pimpl = nullptr;

		return *this;
	}

	ModelHandle::~ModelHandle(void) {
		if (nullptr == this->pimpl) return;

		this->pimpl->m_refCount--;

		if (this->pimpl->m_refCount <= 0) {
			g_logger.putWarn("A model handler's refference all lost without being destroyed: "s + this->pimpl->m_id);
			delete this->pimpl;
			this->pimpl = nullptr;
		}
	}

	void ModelHandle::renderGeneral(const UnilocGeneral& uniloc, const std::list<Actor>& actors) const {
		if (nullptr == this->pimpl->m_model) return;

		for (auto& unit : this->pimpl->m_model->m_renderUnits) {
			unit.m_material.sendUniform(uniloc);
			if (!unit.m_mesh.isReady()) continue;

			for (auto& inst : actors) {
				auto mat = inst.getViewMat();
				glUniformMatrix4fv(uniloc.uModelMat, 1, GL_FALSE, &mat[0][0]);
				unit.m_mesh.draw();
			}
		}
	}

}


// Package
namespace dal {

	void Package::setName(const char* const packageName) {
		this->m_name = packageName;
	}

	void Package::setName(const std::string& packageName) {
		this->m_name = packageName;
	}

	ModelHandle Package::orderModel(const ResourceFilePath& resPath) {
		std::string modelIDStr{ resPath.m_name + resPath.m_ext };

		decltype(this->m_models.end()) iter = this->m_models.find(modelIDStr);
		if (this->m_models.end() != iter) {
			return iter->second;
		}
		else {
			g_logger.putTrace("Load model: "s + resPath.m_package + "::" + resPath.m_dir + modelIDStr);
			ModelHandle handle{ modelIDStr.c_str(), nullptr };
			this->m_models.emplace(modelIDStr, handle);
			return handle;
		}
	}

	ModelHandle Package::buildModel(const buildinfo::ModelDefined& info) {
		auto model = g_modelPool.alloc();
		ModelHandle handle{ info.m_modelID.c_str(), model };
		this->m_models.emplace(info.m_modelID, handle);

		{
			// Render units
			model->m_renderUnits.emplace_back();
			auto& renderUnit = model->m_renderUnits.back();
			renderUnit.m_mesh.buildData(
				info.m_renderUnit.m_mesh.m_vertices.data(), info.m_renderUnit.m_mesh.m_vertices.size(),
				info.m_renderUnit.m_mesh.m_texcoords.data(), info.m_renderUnit.m_mesh.m_texcoords.size(),
				info.m_renderUnit.m_mesh.m_normals.data(), info.m_renderUnit.m_mesh.m_normals.size()
			);

			// Material
			renderUnit.m_material.mSpecularStrength = info.m_renderUnit.m_material.m_specStrength;
			renderUnit.m_material.mShininess = info.m_renderUnit.m_material.m_shininess;
			renderUnit.m_material.mDiffuseColor = info.m_renderUnit.m_material.m_diffuseColor;

			/*
			if (!definedModel.m_renderUnit.m_material.m_diffuseMap.empty()) {
				renderUnit.m_material.setDiffuseMap(m_texMas.request_diffuseMap(definedModel.m_renderUnit.m_material.m_diffuseMap.c_str()));
			}
			*/
		}

		return handle;
	}

}


// ResourceMaster
namespace dal {

	ModelHandle ResourceMaster::orderModel(const char* const packageName_dir_modelID) {
		ResourceFilePath path;
		parseResFilePath(packageName_dir_modelID, path);

		auto& package = this->orderPackage(path.m_package);

		return package.orderModel(path);
	}

	ModelHandle ResourceMaster::buildModel(const buildinfo::ModelDefined& info, const char* const packageName) {
		auto& package = this->orderPackage(packageName);
		return package.buildModel(info);
	}

	// Private

	Package& ResourceMaster::orderPackage(const std::string& packName) {
		std::string packNameStr{ packName };
		
		decltype(this->m_packages.end()) iter = this->m_packages.find(packNameStr);
		if (iter != this->m_packages.end()) {
			return iter->second;
		}
		else { // If not found
			Package package;
			package.setName(packName);
			auto res = this->m_packages.emplace(packName, package);
			return res.first->second;
		}
	}

}