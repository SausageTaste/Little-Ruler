#include "p_scene.h"

#include <unordered_set>
#include <cassert>

#include <glm/gtc/matrix_transform.hpp>

#include "u_objparser.h"
#include "u_fileclass.h"
#include "s_logger_god.h"
#include "u_timer.h"
#include "u_maploader.h"


using namespace std::string_literals;


namespace {

	void buildBoxMeshAABB(std::vector<float>* vert, std::vector<float>* texc, std::vector<float>* norm, glm::vec3 p1, glm::vec3 p2) {
		if (p1.x > p2.x) {
			auto temp = p1.x;
			p1.x = p2.x;
			p2.x = temp;
		}
		if (p1.y > p2.y) {
			auto temp = p1.y;
			p1.y = p2.y;
			p2.y = temp;
		}
		if (p1.z > p2.z) {
			auto temp = p1.z;
			p1.z = p2.z;
			p2.z = temp;
		}

		vert->clear();
		texc->clear();
		norm->clear();

		/* Vertices */ {
			const float vertArr[] = {
				p1.x, p2.y, p1.z,
				p1.x, p2.y, p2.z,
				p2.x, p2.y, p2.z,
				p1.x, p2.y, p1.z,
				p2.x, p2.y, p2.z,
				p2.x, p2.y, p1.z,

				p1.x, p2.y, p2.z,
				p1.x, p1.y, p2.z,
				p2.x, p1.y, p2.z,
				p1.x, p2.y, p2.z,
				p2.x, p1.y, p2.z,
				p2.x, p2.y, p2.z,

				p2.x, p2.y, p2.z,
				p2.x, p1.y, p2.z,
				p2.x, p1.y, p1.z,
				p2.x, p2.y, p2.z,
				p2.x, p1.y, p1.z,
				p2.x, p2.y, p1.z,

				p2.x, p2.y, p1.z,
				p2.x, p1.y, p1.z,
				p1.x, p1.y, p1.z,
				p2.x, p2.y, p1.z,
				p1.x, p1.y, p1.z,
				p1.x, p2.y, p1.z,

				p1.x, p2.y, p1.z,
				p1.x, p1.y, p1.z,
				p1.x, p1.y, p2.z,
				p1.x, p2.y, p1.z,
				p1.x, p1.y, p2.z,
				p1.x, p2.y, p2.z,

				p2.x, p1.y, p1.z,
				p2.x, p1.y, p2.z,
				p1.x, p1.y, p2.z,
				p2.x, p1.y, p1.z,
				p1.x, p1.y, p2.z,
				p1.x, p1.y, p1.z
			};
			const auto vertSize = sizeof(vertArr) / sizeof(float);
			vert->resize(0);
			vert->insert(vert->end(), &vertArr[0], &vertArr[vertSize]);
		}
		/* Textrue Coords */ {
			const float texcArr[] = {
				0, 1,
				0, 0,
				1, 0,
				0, 1,
				1, 0,
				1, 1,

				0, 1,
				0, 0,
				1, 0,
				0, 1,
				1, 0,
				1, 1,

				0, 1,
				0, 0,
				1, 0,
				0, 1,
				1, 0,
				1, 1,

				0, 1,
				0, 0,
				1, 0,
				0, 1,
				1, 0,
				1, 1,

				0, 1,
				0, 0,
				1, 0,
				0, 1,
				1, 0,
				1, 1,

				0, 1,
				0, 0,
				1, 0,
				0, 1,
				1, 0,
				1, 1,
			};
			const auto texcSize = sizeof(texcArr) / sizeof(float);
			texc->resize(0);
			texc->insert(texc->end(), &texcArr[0], &texcArr[texcSize]);
		}
		/* Normals */ {
			const float normArr[] = {
				0, 1, 0,
				0, 1, 0,
				0, 1, 0,
				0, 1, 0,
				0, 1, 0,
				0, 1, 0,

				0, 0, 1,
				0, 0, 1,
				0, 0, 1,
				0, 0, 1,
				0, 0, 1,
				0, 0, 1,

				1, 0, 0,
				1, 0, 0,
				1, 0, 0,
				1, 0, 0,
				1, 0, 0,
				1, 0, 0,

				0, 0, -1,
				0, 0, -1,
				0, 0, -1,
				0, 0, -1,
				0, 0, -1,
				0, 0, -1,

				-1, 0, 0,
				-1, 0, 0,
				-1, 0, 0,
				-1, 0, 0,
				-1, 0, 0,
				-1, 0, 0,

				0, -1, 0,
				0, -1, 0,
				0, -1, 0,
				0, -1, 0,
				0, -1, 0,
				0, -1, 0,
			};
			const auto normSize = sizeof(normArr) / sizeof(float);
			norm->resize(0);
			norm->insert(norm->end(), &normArr[0], &normArr[normSize]);
		}


	}

	const char* const g_objMtlLoadTask_name = "ModelLoadTask";

}


namespace {

	class ModelLoadTask : public dal::iTask {

	public:
		const std::string in_modelName;

		bool out_success;
		dal::ModelInfo out_info;

		dal::ModelInst* const data_coresponding;

	public:
		ModelLoadTask(const char* const modelName, dal::ModelInst* const coresponding)
		:	iTask(g_objMtlLoadTask_name),
			in_modelName(modelName),
			out_success(false),
			data_coresponding(coresponding)
		{

		}

		virtual void start(void) override {
			out_success = dal::parseOBJ_assimp(out_info, ("models/"s + this->in_modelName).c_str());
		}

	};

	std::unordered_set<void*> g_sentTasks_modelLoad;

}

/*
namespace dal {

	bool RenderUnit_Static::isReady(void) const {
		return this->mesh.isReady();
	}

	ActorInfo::ActorInfo(void) : rescale(1.0f) {

	}

	ActorInfo::ActorInfo(glm::vec3 initPos)
		: pos(initPos),
		rescale(1.0f)
	{

	}

	ActorInfo::ActorInfo(const float x, const float y, const float z)
		: pos(x, y, z), rescale(1.0f)
	{

	}

	void ActorInfo::getViewMat(glm::mat4* mat) const {
		auto scaleMat = glm::scale(glm::mat4{ 1.0f }, { rescale, rescale , rescale });
		auto translateMat = glm::translate(glm::mat4{ 1.0f }, this->pos);
		*mat = translateMat * glm::mat4_cast(myQuat) * scaleMat;
	}

	void ActorInfo::rotate(const float v, const glm::vec3& selector) {
		this->myQuat = glm::angleAxis(v, selector) * this->myQuat;
		this->myQuat = glm::normalize(this->myQuat);
	}

}
*/

namespace dal {

	SceneMaster::SceneMaster(TextureMaster& texMas) : m_texMas(texMas) {
		{
			ModelBuildInfo_AABB info;
			info.m_p1 = { -50.0f, -1.0f, -50.0f };
			info.m_p2 = { 50.0f, 0.0f, 50.0f };
			info.m_textureName = "grass1.png";
			info.m_modelName = "floor";
			info.m_texScaleX = 10.0f;
			info.m_texScaleY = 10.0f;
			info.m_instanceInfo.emplace_back();
			info.m_instanceInfo.back().pos = { 0.0f, -2.0f, 0.0f };
			this->addObject(info);
		}

		{
			ModelBuildInfo_Load info;
			info.m_modelName = "palanquin.obj";
			info.m_instanceInfo.emplace_back();
			info.m_instanceInfo.back().pos = { 0.0f, -2.0f, 0.0f };
			this->addObject(info);
		}
		
		{
			ModelBuildInfo_Load info;
			info.m_modelName = "yuri.obj";
			info.m_instanceInfo.emplace_back();
			info.m_instanceInfo.back().pos = { 4.0f, -2.0f, 0.0f };
			info.m_instanceInfo.back().rotate(glm::radians(180.0f), { 0.0f, 1.0f, 0.0f });
			this->addObject(info);
		}
		
		{
			ModelBuildInfo_Load info;
			info.m_modelName = "honoka.obj";
			info.m_instanceInfo.emplace_back();
			info.m_instanceInfo.back().pos = { -4.0f, -2.0f, 0.0f };
			this->addObject(info);
		}

		{
			ModelBuildInfo_Load info;
			info.m_modelName = "honoka_bunny.obj";
			info.m_instanceInfo.emplace_back();
			info.m_instanceInfo.back().pos = { -8.0f, -2.0f, 0.0f };
			this->addObject(info);
		}

		{
			ModelBuildInfo_Load info;
			info.m_modelName = "honoka_apron.obj";
			info.m_instanceInfo.emplace_back();
			info.m_instanceInfo.back().pos = { 8.0f, -2.0f, 0.0f };
			this->addObject(info);
		}

		{
			ModelBuildInfo_Load info;
			info.m_modelName = "honoka_nude.obj";
			info.m_instanceInfo.emplace_back();
			info.m_instanceInfo.back().pos = { 12.0f, -2.0f, 0.0f };
			this->addObject(info);
		}

		{
			ModelBuildInfo_Load info;
			info.m_modelName = "brit.obj";
			info.m_instanceInfo.emplace_back();
			info.m_instanceInfo.back().pos = { -12.0f, -2.0f, 0.0f };
			this->addObject(info);
		}
		
		{
			AssetFileStream file{ "maps/test_level.dlb" };
			const auto bufSize = file.getFileSize();
			std::unique_ptr<uint8_t> buf{ new uint8_t[bufSize] };
			auto res = file.read(buf.get(), bufSize);
			if (!res) throw - 1;
			LoadedMap info;
			info.m_mapName = "test_map";
			res = parseMap_dlb(info, buf.get(), bufSize);
			if (!res) LoggerGod::getinst().putError("Failed level loading test.");
			this->addMapChunk(info);
		}
	}

	SceneMaster::~SceneMaster(void) {

	}

	void SceneMaster::notify(iTask* const task) {
		std::unique_ptr<iTask> taskPtr{ task };

		if (g_sentTasks_modelLoad.find(task) != g_sentTasks_modelLoad.end()) {
			g_sentTasks_modelLoad.erase(task);

			auto loaded = reinterpret_cast<ModelLoadTask*>(task);
			if (true != loaded->out_success) {
				LoggerGod::getinst().putError("Failed to load model: "s + loaded->in_modelName);
				return;
			}

			for (auto& unitInfo : loaded->out_info) {
				loaded->data_coresponding->m_renderUnits.emplace_back();
				auto& unit = loaded->data_coresponding->m_renderUnits.back();

				unit.m_mesh.buildData(
					unitInfo.m_mesh.m_vertices.data(),  unitInfo.m_mesh.m_vertices.size(),
					unitInfo.m_mesh.m_texcoords.data(), unitInfo.m_mesh.m_texcoords.size(),
					unitInfo.m_mesh.m_normals.data(),   unitInfo.m_mesh.m_normals.size()
				);
				unit.m_name = unitInfo.m_name;

				unit.m_material.mDiffuseColor = unitInfo.m_material.m_diffuseColor;
				unit.m_material.mShininess = unitInfo.m_material.m_shininess;
				unit.m_material.mSpecularStrength = unitInfo.m_material.m_specStrength;

				if (!unitInfo.m_material.m_diffuseMap.empty()) {
					auto tex = m_texMas.request_diffuseMap(unitInfo.m_material.m_diffuseMap.c_str());
					unit.m_material.setDiffuseMap(tex);
				}
			}
		}
		else {
			LoggerGod::getinst().putFatal("Not registered task revieved in TextureMaster::notify.");
			throw -1;
		}
	}

	void SceneMaster::renderGeneral(const UnilocGeneral& uniloc) const {
		for (auto& model : m_freeModels) {
			for (auto unit = model.m_renderUnits.rbegin(); unit != model.m_renderUnits.rend(); ++unit) {
				unit->m_material.sendUniform(uniloc);
				if (!unit->m_mesh.isReady()) continue;

				for (auto& inst : model.m_inst) {
					auto mat = inst.getViewMat();
					glUniformMatrix4fv(uniloc.uModelMat, 1, GL_FALSE, &mat[0][0]);
					unit->m_mesh.draw();
				}
			}
		}
	}

	void SceneMaster::renderDepthMp(const UnilocDepthmp& uniloc) const {
		for (auto& model : m_freeModels) {
			for (auto& unit : model.m_renderUnits) {
				for (auto& inst : model.m_inst) {
					auto mat = inst.getViewMat();
					glUniformMatrix4fv(uniloc.uModelMat, 1, GL_FALSE, &mat[0][0]);
					unit.m_mesh.draw();
				}
			}
		}
	}

	void SceneMaster::addObject(const ModelBuildInfo_AABB& info) {
		std::vector<float> vert, texc, norm;
		buildBoxMeshAABB(&vert, &texc, &norm, info.m_p1, info.m_p2);

		m_freeModels.emplace_back();
		auto& modelInst = m_freeModels.back();
		modelInst.m_name = info.m_modelName != nullptr ? info.m_modelName : "__noname__";

		modelInst.m_renderUnits.emplace_back();
		modelInst.m_renderUnits.back().m_mesh.buildData(
			vert.data(), (int)vert.size(),
			texc.data(), (int)texc.size(),
			norm.data(), (int)norm.size()
		);

		modelInst.m_renderUnits.at(0).m_material.mSpecularStrength = info.m_specStrength;
		modelInst.m_renderUnits.at(0).m_material.mShininess = info.m_shininess;
		modelInst.m_renderUnits.at(0).m_material.mTexScaleX = info.m_texScaleX;
		modelInst.m_renderUnits.at(0).m_material.mTexScaleY = info.m_texScaleY;
		if (info.m_textureName != nullptr) {
			modelInst.m_renderUnits.at(0).m_material.setDiffuseMap(m_texMas.request_diffuseMap(info.m_textureName));
		}

		modelInst.m_inst.assign(info.m_instanceInfo.begin(), info.m_instanceInfo.end());
	}

	void SceneMaster::addObject(const ModelBuildInfo_Load& info) {
		m_freeModels.emplace_back();
		auto& modelInst = m_freeModels.back();
		modelInst.m_name = info.m_modelName;
		modelInst.m_inst.assign(info.m_instanceInfo.begin(), info.m_instanceInfo.end());

		auto task = new ModelLoadTask(info.m_modelName, &modelInst);
		g_sentTasks_modelLoad.insert(task);
		TaskGod::getinst().orderTask(task, this);
	}

	void SceneMaster::addMapChunk(const LoadedMap& map) {
		this->m_mapChunks.emplace_back();
		auto& newMap = this->m_mapChunks.back();

		for (auto& definedModel : map.m_definedModels) {
			newMap.m_actors.emplace_back();
			auto& model = newMap.m_actors.back();

			// ID
			model.m_name = definedModel.m_modelID;

			// Actors
			model.m_inst.assign(definedModel.m_actors.begin(), definedModel.m_actors.end());

			// Render units
			model.m_renderUnits.emplace_back();
			auto& renderUnit = model.m_renderUnits.back();
			renderUnit.m_mesh.buildData(
				definedModel.m_renderUnit.m_mesh.m_vertices.data(), definedModel.m_renderUnit.m_mesh.m_vertices.size(),
				definedModel.m_renderUnit.m_mesh.m_texcoords.data(), definedModel.m_renderUnit.m_mesh.m_texcoords.size(),
				definedModel.m_renderUnit.m_mesh.m_normals.data(), definedModel.m_renderUnit.m_mesh.m_normals.size()
			);

			// Material
			renderUnit.m_material.mSpecularStrength = definedModel.m_renderUnit.m_material.m_specStrength;
			renderUnit.m_material.mShininess = definedModel.m_renderUnit.m_material.m_shininess;
			if (!definedModel.m_renderUnit.m_material.m_diffuseMap.empty()) {
				renderUnit.m_material.setDiffuseMap(m_texMas.request_diffuseMap(definedModel.m_renderUnit.m_material.m_diffuseMap.c_str()));
			}
		}

		for (auto& importedModel : map.m_importedModels) {
			newMap.m_actors.emplace_back();
			auto& model = newMap.m_actors.back();

			model.m_name = importedModel.m_modelID;
			model.m_inst.assign(importedModel.m_actors.begin(), importedModel.m_actors.end());

			auto task = new ModelLoadTask(importedModel.m_modelID.c_str(), &model);
			g_sentTasks_modelLoad.insert(task);
			TaskGod::getinst().orderTask(task, this);
		}
	}

	// Private

	bool SceneMaster::findModel(const char* const name, ModelInst** const model, const char** const level) {
		for (auto& modelInst : m_freeModels) {
			if (modelInst.m_name == name) {
				if (model != nullptr) *model = &modelInst;
				if (level != nullptr) *level = nullptr;
				return true;
			}
		}

		if (model != nullptr) *model = nullptr;
		if (level != nullptr) *level = nullptr;
		return false;
	}

}