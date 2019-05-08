#include "p_resource.h"

#include "p_dalopengl.h"
#include "s_logger_god.h"
#include "u_fileclass.h"
#include "s_threader.h"
#include "u_objparser.h"


using namespace std::string_literals;


namespace {

	struct RenderUnit {
		std::string m_meshName;
		dal::MeshStatic m_mesh;
		dal::Material2 m_material;
	};

}


namespace {

	class LoadTask_Texture : public dal::ITask {

	public:
		dal::ResourceFilePath in_id;

		dal::loadedinfo::ImageFileData out_img;

		bool out_success;

		virtual void start(void) override {
			std::string filePath{ in_id.m_package + in_id.m_dir + in_id.m_name + in_id.m_ext };

			out_success = dal::filec::getResource_image(filePath.c_str(), out_img);
			return;
		}

	};

	class LoadTask_Model : public dal::ITask {

	public:
		const std::string in_modelID;

		bool out_success;
		dal::ModelInfo out_info;

		dal::ModelHandle data_coresponding;
		dal::Package* const data_package;

	public:
		LoadTask_Model(const std::string& modelID, dal::ModelHandle const coresponding, dal::Package* const package)
		:	in_modelID(modelID),
			out_success(false),
			data_coresponding(coresponding),
			data_package(package)
		{

		}

		virtual void start(void) override {
			dal::ResourceFilePath path;
			dal::parseResFilePath(in_modelID.c_str(), path);
			if ("asset" != path.m_package) {
				out_success = false;
				return;
			}
			auto modelPath = "models/"s + path.m_dir + path.m_name + path.m_ext;
			out_success = dal::parseOBJ_assimp(out_info, (modelPath).c_str());
		}

	};


	std::unordered_set<void*> g_sentTasks_texture;

	std::unordered_set<void*> g_sentTasks_model;

}


namespace dal {

	struct Model {
		std::vector<RenderUnit> m_renderUnits;
	};

}


namespace {

	auto& g_logger = dal::LoggerGod::getinst();

	dal::StaticPool<dal::Model, 100> g_modelPool;
	dal::StaticPool<dal::Texture, 100> g_texturePool;

}


// TextureHandle
namespace dal {

	struct TextureHandle2::Pimpl {
		std::string m_id;
		Texture* m_tex = nullptr;
		unsigned int m_refCount = 1;
	};


	TextureHandle2::TextureHandle2(void) {

	}
	TextureHandle2::TextureHandle2(const char* const texID, Texture* const texture)
	:	pimpl(new Pimpl) 
	{
		this->pimpl->m_tex = texture;
		this->pimpl->m_id = texID;
	}
	TextureHandle2::TextureHandle2(const TextureHandle2& other) : pimpl(other.pimpl) {
		this->pimpl->m_refCount++;
	}
	TextureHandle2::TextureHandle2(TextureHandle2&& other) noexcept {
		auto temp = this->pimpl;
		this->pimpl = other.pimpl;
		other.pimpl = temp;
	}
	TextureHandle2& TextureHandle2::operator=(const TextureHandle2& other) {
		this->pimpl = other.pimpl;
		this->pimpl->m_refCount++;

		return *this;
	}
	TextureHandle2& TextureHandle2::operator=(TextureHandle2&& other) noexcept {
		auto temp = this->pimpl;
		this->pimpl = other.pimpl;
		other.pimpl = temp;

		return *this;
	}
	TextureHandle2::~TextureHandle2(void) {
		if (nullptr == this->pimpl) return;

		this->pimpl->m_refCount--;

		if (this->pimpl->m_refCount <= 0) {
			if (nullptr != this->pimpl->m_tex) {
				g_logger.putWarn("A texture handler's refference all lost without being destroyed: "s + this->pimpl->m_id);
				this->destroyTexture();
			}
			
			delete this->pimpl;
			this->pimpl = nullptr;
		}
	}

	bool TextureHandle2::isReady(void) const {
		if (nullptr == this->pimpl) return false;
		if (nullptr == this->pimpl->m_tex) return false;
		if (!this->pimpl->m_tex->isInitiated()) return false;

		return true;
	}

	void TextureHandle2::sendUniform(const GLint uniloc_sampler, const GLint uniloc_hasTex, const unsigned int index) const {
		if (this->isReady()) {
			glUniform1i(uniloc_hasTex, 1);
			this->pimpl->m_tex->sendUniform(uniloc_sampler, index);
		}
		else {
			glUniform1i(uniloc_hasTex, 0);
		}
	}

	void TextureHandle2::destroyTexture(void) {
		if (nullptr == this->pimpl->m_tex) return;

		g_texturePool.free(this->pimpl->m_tex);
		this->pimpl->m_tex = nullptr;
	}

}


namespace dal {

	void Material2::setTexScale(float x, float y) {
		this->m_texScale = { x, y };
	}

	void Material2::setDiffuseMap(TextureHandle2 tex) {
		this->m_diffuseMap = tex;
	}

	void Material2::sendUniform(const UnilocGeneral& uniloc) const {
		glUniform1f(uniloc.uShininess, this->m_shininess);
		glUniform1f(uniloc.uSpecularStrength, this->m_specularStrength);

		glUniform1f(uniloc.uTexScaleX, this->m_texScale.x);
		glUniform1f(uniloc.uTexScaleY, this->m_texScale.y);

		glUniform3f(uniloc.uDiffuseColor, this->m_diffuseColor.x, this->m_diffuseColor.y, this->m_diffuseColor.z);

		this->m_diffuseMap.sendUniform(uniloc.uDiffuseMap, uniloc.uHasDiffuseMap, 0);
	}

}


// ModelHandle
namespace dal {

	struct ModelHandle::Pimpl {
		std::string m_id;
		Model* m_model = nullptr;
		unsigned int m_refCount = 1;
	};


	ModelHandle::ModelHandle(void) {

	}
	ModelHandle::ModelHandle(const char* const modelID, Model* const model) : pimpl(new Pimpl) {
		this->pimpl->m_model = model;
	}
	ModelHandle::ModelHandle(const ModelHandle& other) {
		this->pimpl = other.pimpl;
		this->pimpl->m_refCount++;
	}
	ModelHandle::ModelHandle(ModelHandle&& other) noexcept {
		auto temp = this->pimpl;
		this->pimpl = other.pimpl;
		other.pimpl = temp;
	}
	ModelHandle& ModelHandle::operator=(const ModelHandle& other) {
		this->pimpl = other.pimpl;
		this->pimpl->m_refCount++;

		return *this;
	}
	ModelHandle& ModelHandle::operator=(ModelHandle&& other) noexcept {
		auto temp = this->pimpl;
		this->pimpl = other.pimpl;
		other.pimpl = temp;

		return *this;
	}
	ModelHandle::~ModelHandle(void) {
		if (nullptr == this->pimpl) return;

		this->pimpl->m_refCount--;

		if (this->pimpl->m_refCount <= 0) {
			if (nullptr != this->pimpl->m_model) {
				g_logger.putWarn("A model handler's refference all lost without being destroyed: "s + this->pimpl->m_id);
				this->destroyModel();
			}
			
			delete this->pimpl;
			this->pimpl = nullptr;
		}
	}

	bool ModelHandle::isReady(void) const {
		if (nullptr == this->pimpl) return false;
		if (nullptr == this->pimpl->m_model) return false;
		
		return true;
	}

	void ModelHandle::renderGeneral(const UnilocGeneral& uniloc, const std::list<Actor>& actors) const {
		if (!this->isReady()) return;

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

	void ModelHandle::renderDepthMap(const UnilocDepthmp& uniloc, const std::list<Actor>& actors) const {
		if (!this->isReady()) return;

		for (auto& unit : this->pimpl->m_model->m_renderUnits) {
			if (!unit.m_mesh.isReady()) continue;

			for (auto& inst : actors) {
				auto mat = inst.getViewMat();
				glUniformMatrix4fv(uniloc.uModelMat, 1, GL_FALSE, &mat[0][0]);
				unit.m_mesh.draw();
			}
		}
	}

	void ModelHandle::destroyModel(void) {
		if (nullptr == this->pimpl->m_model) return;

		g_modelPool.free(this->pimpl->m_model);
		this->pimpl->m_model = nullptr;
	}

	Model* ModelHandle::replace(Model* model) {
		if (nullptr == this->pimpl) {
			this->pimpl = new Pimpl;
			this->pimpl->m_model = model;
			return nullptr;
		}
		else {
			auto tmp = this->pimpl->m_model;
			this->pimpl->m_model = model;
			return tmp;
		}
	}

	std::string ModelHandle::replace(const std::string& id) {
		if (nullptr == this->pimpl) {
			this->pimpl = new Pimpl;
			this->pimpl->m_id = id;
			return "";
		}
		else {
			auto tmp = this->pimpl->m_id;
			this->pimpl->m_id = id;
			return tmp;
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

	ModelHandle Package::orderModel(const ResourceFilePath& resPath, ResourceMaster* const resMas) {
		std::string modelIDStr{ resPath.m_name + resPath.m_ext };

		decltype(this->m_models.end()) iter = this->m_models.find(modelIDStr);
		if (this->m_models.end() != iter) {
			return iter->second;
		}
		else {
			const auto modelID{ resPath.m_package + "::" + resPath.m_dir + modelIDStr };
			g_logger.putTrace("Load model: "s + resPath.m_package + "::" + resPath.m_dir + modelIDStr);
			ModelHandle handle{ modelIDStr.c_str(), nullptr };

			auto task = new LoadTask_Model{ modelID, handle, this };
			g_sentTasks_model.insert(task);
			TaskGod::getinst().orderTask(task, resMas);
			
			this->m_models.emplace(modelIDStr, handle);
			return handle;
		}
	}

	ModelHandle Package::buildModel(const loadedinfo::ModelDefined& info) {
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
			renderUnit.m_material.m_specularStrength = info.m_renderUnit.m_material.m_specStrength;
			renderUnit.m_material.m_shininess = info.m_renderUnit.m_material.m_shininess;
			renderUnit.m_material.m_diffuseColor = info.m_renderUnit.m_material.m_diffuseColor;

			
			if (!info.m_renderUnit.m_material.m_diffuseMap.empty()) {
				auto texHandle = this->orderDiffuseMap(info.m_renderUnit.m_material.m_diffuseMap.c_str());
				renderUnit.m_material.setDiffuseMap(texHandle);
			}
			
		}

		return handle;
	}

	TextureHandle2 Package::orderDiffuseMap(const char* const texID) {
		auto iter = this->m_textures.find(texID);
		if (this->m_textures.end() == iter) {
			loadedinfo::ImageFileData img;
			std::string filePath{ this->m_name + "::texture/"s + texID };
			const auto res = dal::filec::getResource_image(filePath.c_str(), img);
			if (!res) throw - 1;
			return this->buildDiffuseMap(texID, img);
		}
		else {
			return iter->second;
		}
	}

	TextureHandle2 Package::buildDiffuseMap(const char* const texID, const loadedinfo::ImageFileData& info) {
		auto tex = g_texturePool.alloc();
		tex->init_diffueMap(info.m_buf.data(), info.m_width, info.m_height);
		TextureHandle2 handle{ texID, tex };
		this->m_textures.emplace(texID, handle);
		return handle;
	}

	void Package::clear(void) {
		for (auto& model : this->m_models) {
			model.second.destroyModel();
		}
		this->m_models.clear();

		for (auto& tex : this->m_textures) {
			tex.second.destroyTexture();
		}
		this->m_textures.clear();
	}

}


// ResourceMaster
namespace dal {

	ResourceMaster::~ResourceMaster(void) {
		for (auto& package : this->m_packages) {
			package.second.clear();
		}
		this->m_packages.clear();
	}

	void ResourceMaster::notifyTask(ITask* const task) {
		std::unique_ptr<ITask> safeHoho{ task };

		if (g_sentTasks_model.find(task) != g_sentTasks_model.end()) {
			g_sentTasks_model.erase(task);

			auto loaded = reinterpret_cast<LoadTask_Model*>(task);
			if (!loaded->out_success) {
				LoggerGod::getinst().putError("Failed to load model: "s + loaded->in_modelID);
				throw - 1;
			}

			auto model = g_modelPool.alloc();
			auto shouldBeNULL = loaded->data_coresponding.replace(model);
			assert(nullptr == shouldBeNULL);
			auto shouldBeEmptyStr = loaded->data_coresponding.replace(loaded->in_modelID);
			assert(shouldBeEmptyStr.empty());

			for (auto& unitInfo : loaded->out_info) {
				model->m_renderUnits.emplace_back();
				auto& unit = model->m_renderUnits.back();

				unit.m_mesh.buildData(
					unitInfo.m_mesh.m_vertices.data(), unitInfo.m_mesh.m_vertices.size(),
					unitInfo.m_mesh.m_texcoords.data(), unitInfo.m_mesh.m_texcoords.size(),
					unitInfo.m_mesh.m_normals.data(), unitInfo.m_mesh.m_normals.size()
				);
				unit.m_meshName = unitInfo.m_name;

				unit.m_material.m_diffuseColor = unitInfo.m_material.m_diffuseColor;
				unit.m_material.m_shininess = unitInfo.m_material.m_shininess;
				unit.m_material.m_specularStrength = unitInfo.m_material.m_specStrength;

				if (!unitInfo.m_material.m_diffuseMap.empty()) {
					auto texHandle = loaded->data_package->orderDiffuseMap(unitInfo.m_material.m_diffuseMap.c_str());
					unit.m_material.setDiffuseMap(texHandle);
				}
			}
		}
		else {
			g_logger.putWarn("ResourceMaster got a task that it doesn't know.");
		}
	}

	ModelHandle ResourceMaster::orderModel(const char* const packageName_dir_modelID) {
		ResourceFilePath path;
		parseResFilePath(packageName_dir_modelID, path);

		auto& package = this->orderPackage(path.m_package);

		return package.orderModel(path, this);
	}

	ModelHandle ResourceMaster::buildModel(const loadedinfo::ModelDefined& info, const char* const packageName) {
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