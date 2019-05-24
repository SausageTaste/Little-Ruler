#include "p_resource.h"

#include <unordered_set>

#include "p_dalopengl.h"
#include "s_logger_god.h"
#include "u_fileclass.h"
#include "s_threader.h"
#include "u_objparser.h"

#define BLOCKY_TEXTURE 1


using namespace std::string_literals;


// Misc
namespace {

	auto& g_logger = dal::LoggerGod::getinst();

}


// Render Unit
namespace {

	struct RenderUnit {
		std::string m_meshName;
		dal::MeshStatic m_mesh;
		dal::Material m_material;
	};

}


// Texture
namespace dal {

	Texture::Texture(const GLuint id)
		: m_texID(id)
	{

	}

	Texture::Texture(Texture&& other) noexcept {
		this->m_texID = other.m_texID;
		other.m_texID = 0;
	}

	Texture& Texture::operator=(Texture&& other) noexcept {
		this->m_texID = other.m_texID;
		other.m_texID = 0;

		return *this;
	}

	Texture::~Texture(void) {
		if ( this->isReady() ) this->deleteTex();
	}


	void Texture::init_diffueMap(const uint8_t* const image, const unsigned int width, const unsigned int height) {
		glPixelStorei(GL_UNPACK_ALIGNMENT, 0);

		this->genTexture("init_diffueMap");

		glBindTexture(GL_TEXTURE_2D, m_texID);

#if BLOCKY_TEXTURE == 0
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#endif
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		glGenerateMipmap(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void Texture::init_diffueMap3(const uint8_t* const image, const unsigned int width, const unsigned int height) {
		glPixelStorei(GL_UNPACK_ALIGNMENT, 0);

		this->genTexture("init_diffueMap");

		glBindTexture(GL_TEXTURE_2D, m_texID);

#if BLOCKY_TEXTURE == 0
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#endif
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		glGenerateMipmap(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void Texture::init_depthMap(const unsigned int width, const unsigned int height) {
		glPixelStorei(GL_UNPACK_ALIGNMENT, 0);

		this->genTexture("init_depthMap");

		glBindTexture(GL_TEXTURE_2D, m_texID);
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);
		} glBindTexture(GL_TEXTURE_2D, 0);
	}

	void Texture::init_maskMap(const uint8_t* const image, const unsigned int width, const unsigned int height) {
		this->genTexture("init_maskMap");

		glBindTexture(GL_TEXTURE_2D, m_texID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, image);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	void Texture::deleteTex(void) {
		glDeleteTextures(1, &this->m_texID);
		this->m_texID = 0;
	}


	void Texture::sendUniform(const GLint uniloc_sampler, const GLint uniloc_has, const unsigned int index) const {
		if ( this->isReady() ) {
			glUniform1i(uniloc_has, 1);
			glActiveTexture(GL_TEXTURE0 + index);
			glBindTexture(GL_TEXTURE_2D, this->m_texID);
			glUniform1i(uniloc_sampler, index);
		}
		else {
			glUniform1i(uniloc_has, 0);
		}
	}

	bool Texture::isReady(void) const {
		return this->m_texID != 0;
	}

	// Getters

	GLuint Texture::get(void) {
		return m_texID;
	}

	// Private

	void Texture::genTexture(const char* const str4Log) {
		glGenTextures(1, &m_texID);
		if ( m_texID == 0 ) {
			dalAbort("Failed to init dal::Texture::init_depthMap::"s + str4Log);
		}
	}

}


// Tasks
namespace {

	class LoadTask_Texture : public dal::ITask {

	public:
		const dal::ResourceID in_texID;

		dal::loadedinfo::ImageFileData out_img;

		bool out_success = false;

		dal::Texture* data_handle;

	public:
		LoadTask_Texture(const dal::ResourceID& texID, dal::Texture* const handle)
		:	in_texID(texID),
			data_handle(handle)
		{

		}

		virtual void start(void) override {
			dal::ResourceID path;
			if ("asset" != this->in_texID.getPackage()) {
				g_logger.putError("Other packages than 'asset' are not supported yes.", __LINE__, __func__, __FILE__);
				out_success = false;
				return;
			}

			out_success = dal::futil::getRes_image(in_texID, out_img);
			return;
		}

	};


	class LoadTask_Model : public dal::ITask {

	public:
		const dal::ResourceID in_modelID;

		bool out_success;
		dal::ModelInfo out_info;

		dal::ModelHandle data_coresponding;
		dal::Package* const data_package;

	public:
		LoadTask_Model(const dal::ResourceID& modelID, dal::ModelHandle const coresponding, dal::Package* const package)
		:	in_modelID(modelID),
			out_success(false),
			data_coresponding(coresponding),
			data_package(package)
		{

		}

		virtual void start(void) override {
			if ("asset" != in_modelID.getPackage()) {
				out_success = false;
				return;
			}

			out_success = dal::parseOBJ_assimp(out_info, this->in_modelID);
		}

	};


	std::unordered_set<void*> g_sentTasks_texture;

	std::unordered_set<void*> g_sentTasks_model;

}


// Model
namespace dal {

	struct Model {
		std::vector<RenderUnit> m_renderUnits;
	};

}


// Pools
namespace {

	dal::StaticPool<dal::Model, 20> g_modelPool;
	dal::StaticPool<dal::Texture, 200> g_texturePool;

}


// Material
namespace dal {

	void Material::setTexScale(float x, float y) {
		this->m_texScale = { x, y };
	}

	void Material::setDiffuseMap(Texture* const tex) {
		this->m_diffuseMap = tex;
	}

	void Material::sendUniform(const UnilocGeneral& uniloc) const {
		glUniform1f(uniloc.uShininess, this->m_shininess);
		glUniform1f(uniloc.uSpecularStrength, this->m_specularStrength);

		glUniform1f(uniloc.uTexScaleX, this->m_texScale.x);
		glUniform1f(uniloc.uTexScaleY, this->m_texScale.y);

		glUniform3f(uniloc.uDiffuseColor, this->m_diffuseColor.x, this->m_diffuseColor.y, this->m_diffuseColor.z);

		if ( nullptr == this->m_diffuseMap ) {
			glUniform1i(uniloc.uHasDiffuseMap, 0);
		}
		else {
			this->m_diffuseMap->sendUniform(uniloc.uDiffuseMap, uniloc.uHasDiffuseMap, 0);
		}
	}

	void Material::sendUniform(const UnilocWaterry& uniloc) const {
		glUniform1f(uniloc.uShininess, this->m_shininess);
		glUniform1f(uniloc.uSpecularStrength, this->m_specularStrength);

		glUniform1f(uniloc.uTexScaleX, this->m_texScale.x);
		glUniform1f(uniloc.uTexScaleY, this->m_texScale.y);

		glUniform3f(uniloc.uDiffuseColor, this->m_diffuseColor.x, this->m_diffuseColor.y, this->m_diffuseColor.z);

		if ( nullptr == this->m_diffuseMap ) {
			glUniform1i(uniloc.uHasDiffuseMap, 0);
		}
		else {
			this->m_diffuseMap->sendUniform(uniloc.uDiffuseMap, uniloc.uHasDiffuseMap, 0);
		}
	}

}


// Model Handle
namespace dal {

	struct ModelHandle::Pimpl {
		std::string m_id;
		Model* m_model = nullptr;
		unsigned int m_refCount = 1;
	};


	ModelHandle::ModelHandle(void) {

	}
	ModelHandle::ModelHandle(const std::string& modelID, Model* const model) : pimpl(new Pimpl) {
		this->pimpl->m_id = modelID;
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
				g_logger.putWarn("A model handler's refference all lost without being destroyed: "s + this->pimpl->m_id, __LINE__, __func__, __FILE__);
				this->destroyModel();
			}
			
			delete this->pimpl;
			this->pimpl = nullptr;
		}
	}

	bool ModelHandle::operator==(const ModelHandle& other) const {
		return this->pimpl == other.pimpl;
	}

	bool ModelHandle::isReady(void) const {
		if (nullptr == this->pimpl) return false;
		if (nullptr == this->pimpl->m_model) return false;
		
		return true;
	}

	unsigned int ModelHandle::getRefCount(void) const {
		if (nullptr == this->pimpl) return 0;

		return this->pimpl->m_refCount;
	}

	void ModelHandle::renderGeneral(const UnilocGeneral& uniloc, const std::list<ActorInfo>& actors) const {
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

	void ModelHandle::renderDepthMap(const UnilocDepthmp& uniloc, const std::list<ActorInfo>& actors) const {
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

		if (this->pimpl->m_refCount > 1) {
			g_logger.putWarn("Destroying model handle with ref count: " + std::to_string(this->pimpl->m_refCount), __LINE__, __func__, __FILE__);
		}

		for (auto& unit : this->pimpl->m_model->m_renderUnits) {
			unit.m_mesh.destroyData();
		}

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

	void Package::ResourceReport::print(void) const {
		g_logger.putInfo("Package : "s + m_packageName, __LINE__, __func__, __FILE__);
		g_logger.putInfo("\tModels", __LINE__, __func__, __FILE__);

		for (auto& x : m_models) {
			g_logger.putInfo("\t\t"s + x.first + " (" + std::to_string(x.second) + ")", __LINE__, __func__, __FILE__);
		}

		g_logger.putInfo("\tTextures", __LINE__, __func__, __FILE__);

		for (auto& x : m_textures) {
			g_logger.putInfo("\t\t"s + x.first + " (" + std::to_string(x.second) + ")", __LINE__, __func__, __FILE__);
		}
	}

	std::string Package::ResourceReport::getStr(void) const {
		std::string result;

		result.append("Package : "s + m_packageName + '\n');
		result.append("\tModels"s + '\n');

		for (auto& x : m_models) {
			result.append("\t\t"s + x.first + " (" + std::to_string(x.second) + ")" + '\n');
		}

		result.append("\tTextures"s + '\n');

		for (auto& x : m_textures) {
			result.append("\t\t"s + x.first + " (" + std::to_string(x.second) + ")" + '\n');
		}

		return result;
	}


	void Package::setName(const char* const packageName) {
		this->m_name = packageName;
	}

	void Package::setName(const std::string& packageName) {
		this->m_name = packageName;
	}

	ModelHandle Package::orderModel(const ResourceID& resPath, ResourceMaster* const resMas) {
		std::string modelIDStr{ resPath.makeFileName() };

		decltype(this->m_models.end()) iter = this->m_models.find(modelIDStr);
		if (this->m_models.end() != iter) {
			return iter->second;
		}
		else {
			const auto modelID{ resPath.makeIDStr() };
			ModelHandle handle{ modelIDStr.c_str(), nullptr };

			ResourceID idWithPackage{ this->m_name, resPath.getOptionalDir(), resPath.getBareName(), resPath.getExt() };
			auto task = new LoadTask_Model{ idWithPackage, handle, this };
			g_sentTasks_model.insert(task);
			TaskGod::getinst().orderTask(task, resMas);
			
			this->m_models.emplace(modelIDStr, handle);
			return handle;
		}
	}

	ModelHandle Package::buildModel(const loadedinfo::ModelDefined& info, ResourceMaster* const resMas) {
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
				auto texHandle = this->orderDiffuseMap(info.m_renderUnit.m_material.m_diffuseMap.c_str(), resMas);
				renderUnit.m_material.setDiffuseMap(texHandle);
			}
			
		}

		return handle;
	}

	Texture* Package::orderDiffuseMap(const ResourceID& texID, ResourceMaster* const resMas) {
		auto iter = this->m_textures.find(texID.makeFileName());
		if ( this->m_textures.end() == iter ) {
			auto texture = g_texturePool.alloc();
			this->m_textures.emplace(texID.makeFileName(), TextureManageInfo{ texture, 2 });  // ref count is 2 because of return and task.

			ResourceID idWithPackage{ this->m_name, texID.getOptionalDir(), texID.getBareName(), texID.getExt() };
			auto task = new LoadTask_Texture(idWithPackage, texture);
			g_sentTasks_texture.insert(task);
			TaskGod::getinst().orderTask(task, resMas);

			return texture;
		}
		else {
			iter->second.m_refCount++;
			return iter->second.m_tex;
		}
	}

	Texture* Package::buildDiffuseMap(const ResourceID& texID, const loadedinfo::ImageFileData& info) {
		auto tex = g_texturePool.alloc();

		if (3 == info.m_pixSize) {
			tex->init_diffueMap3(info.m_buf.data(), info.m_width, info.m_height);
		}
		else if (4 == info.m_pixSize) {
			tex->init_diffueMap(info.m_buf.data(), info.m_width, info.m_height);
		}
		else {
			g_logger.putError("Not supported pixel size: "s + texID.makeIDStr() + ", " + std::to_string(info.m_pixSize), __LINE__, __func__, __FILE__);
		}
		
		this->m_textures.emplace(texID.makeFileName(), TextureManageInfo{ tex, 1 });
		return tex;
	}

	void Package::getResReport(ResourceReport& report) const {
		report.m_packageName = this->m_name;

		report.m_models.clear();
		report.m_models.reserve(this->m_models.size());
		for (auto& x : this->m_models) {
			report.m_models.emplace_back(x.first, x.second.getRefCount());
		}

		report.m_textures.clear();
		report.m_textures.reserve(this->m_textures.size());
		for (auto& x : this->m_textures) {
			report.m_textures.emplace_back(x.first, x.second.m_refCount);
		}
	}

	void Package::clear(void) {
		for (auto& model : this->m_models) {
			model.second.destroyModel();
		}
		this->m_models.clear();

		for (auto& tex : this->m_textures) {
			tex.second.m_tex->deleteTex();
			g_texturePool.free(tex.second.m_tex);
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

	void ResourceMaster::notifyTask(std::unique_ptr<ITask> task) {
		if (nullptr == task) {
			dalAbort("ResourceMaster::notifyTask has got a nullptr. Why??");
		}

		if (g_sentTasks_model.find(task.get()) != g_sentTasks_model.end()) {
			g_sentTasks_model.erase(task.get());

			auto loaded = reinterpret_cast<LoadTask_Model*>(task.get());
			if (!loaded->out_success) {
				dalAbort("Failed to load model: "s + loaded->in_modelID.makeIDStr());
			}

			auto model = g_modelPool.alloc();
			auto shouldBeNULL = loaded->data_coresponding.replace(model);
			assert(nullptr == shouldBeNULL);

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
					auto texHandle = loaded->data_package->orderDiffuseMap(unitInfo.m_material.m_diffuseMap, this);
					unit.m_material.setDiffuseMap(texHandle);
				}
			}
		}
		else if (g_sentTasks_texture.find(task.get()) != g_sentTasks_texture.end()) {
			g_sentTasks_texture.erase(task.get());

			auto loaded = reinterpret_cast<LoadTask_Texture*>(task.get());
			if (!loaded->out_success) {
				dalAbort("Failed to load texture: "s + loaded->in_texID.makeIDStr());
			}

			if (loaded->out_img.m_pixSize == 3) {
				loaded->data_handle->init_diffueMap3(loaded->out_img.m_buf.data(), loaded->out_img.m_width, loaded->out_img.m_height);
			}
			else if (loaded->out_img.m_pixSize == 4) {
				loaded->data_handle->init_diffueMap(loaded->out_img.m_buf.data(), loaded->out_img.m_width, loaded->out_img.m_height);
			}
			else {
				dalAbort("Unknown pix size: "s + std::to_string(loaded->out_img.m_pixSize));
			}
		}
		else {
			g_logger.putWarn("ResourceMaster got a task that it doesn't know.", __LINE__, __func__, __FILE__);
		}
	}

	ModelHandle ResourceMaster::orderModel(const ResourceID& resID) {
		auto& package = this->orderPackage(resID.getPackage());

		return package.orderModel(resID, this);
	}

	ModelHandle ResourceMaster::buildModel(const loadedinfo::ModelDefined& info, const char* const packageName) {
		auto& package = this->orderPackage(packageName);
		return package.buildModel(info, this);
	}

	size_t ResourceMaster::getResReports(std::vector<Package::ResourceReport>& reports) const {
		reports.clear();
		reports.resize(this->m_packages.size());

		int i = 0;
		for (auto& x : this->m_packages) {
			x.second.getResReport(reports[i]);
			i++;
		}

		return reports.size();
	}

	// Static

	Texture* ResourceMaster::getUniqueTexture(void) {
		return g_texturePool.alloc();
	}

	void ResourceMaster::dumpUniqueTexture(Texture* const tex) {
		g_texturePool.free(tex);
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