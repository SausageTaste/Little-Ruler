#include "p_scene.h"

#include <unordered_set>
#include <cassert>

#include <glm/gtc/matrix_transform.hpp>

#include "u_objparser.h"
#include "u_fileclass.h"
#include "s_logger_god.h"
#include "u_timer.h"


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

	const char* const g_objMtlLoadTask_name = "ObjMtlLoadTask";

}


namespace {

	class ObjMtlLoadTask : public dal::iTask {

	public:
		// Inputs
		std::string in_objName;
		
		// Outputs
		bool out_success;

		dal::OBJInfo_old out_infoObj;
		dal::MTLInfo_old out_infoMtl;

	public:
		ObjMtlLoadTask(const char* const objName)
		:	iTask(g_objMtlLoadTask_name),
			in_objName(objName),
			out_success(false)
		{

		}

		virtual void start(void) override {
			/* OBJ */ {
				dal::AssetFileIn file;
				if (file.open(("models/"s + this->in_objName).c_str()) == false) goto errorFin;

				const auto bufSize = file.getFileSize();
				if (bufSize <= 0) goto errorFin;

				auto buf = std::unique_ptr<uint8_t>(new uint8_t[bufSize]);
				if (file.read(buf.get(), bufSize) == false) goto errorFin;

				file.close();
				if (dal::parseOBJ(&this->out_infoObj, buf.get(), bufSize) == false) goto errorFin;

				dal::ModelInfo modelInfo;
				dal::parseOBJ_assimp(modelInfo, buf.get(), bufSize);
			}

			/* MTL */ {
				for (auto& mtlName : this->out_infoObj.mMaterialFiles) {
					dal::AssetFileIn file;
					if (file.open(("models/"s + mtlName).c_str()) == false) goto errorFin;

					const auto bufSize = file.getFileSize();
					if (bufSize <= 0) goto errorFin;

					auto buf = std::unique_ptr<uint8_t>(new uint8_t[bufSize]);
					if (file.read(buf.get(), bufSize) == false) goto errorFin;

					file.close();
					if (dal::parseMTL(&this->out_infoMtl, buf.get(), bufSize) == false) goto errorFin;
				}
			}

			out_success = true;
			return;

		errorFin:
			out_success = false;
			return;
		}

	};

	std::unordered_set<void*> g_sentTasks_objLoad;

}


namespace dal {

	bool RenderUnit_Static::isReady(void) const {
		return this->mesh.isReady();
	}

	ModelInstanceInfo::ModelInstanceInfo(void) : rescale(1.0f) {

	}

	ModelInstanceInfo::ModelInstanceInfo(glm::vec3 initPos)
		: pos(initPos),
		rescale(1.0f)
	{

	}

	ModelInstanceInfo::ModelInstanceInfo(const float x, const float y, const float z)
		: pos(x, y, z), rescale(1.0f)
	{

	}

	void ModelInstanceInfo::getViewMat(glm::mat4* mat) const {
		auto scaleMat = glm::scale(glm::mat4{ 1.0f }, { rescale, rescale , rescale });
		auto translateMat = glm::translate(glm::mat4{ 1.0f }, this->pos);
		*mat = translateMat * glm::mat4_cast(myQuat) * scaleMat;
	}

	void ModelInstanceInfo::rotate(const float v, const glm::vec3& selector) {
		this->myQuat = glm::angleAxis(v, selector) * this->myQuat;
		this->myQuat = glm::normalize(this->myQuat);
	}

}


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
			info.m_instanceInfo.emplace_back(0.0f, -2.0f, 0.0f);
			info.m_instanceInfo.back().rescale = 3.0f;
			this->addObject(info);
		}

		{
			ModelBuildInfo_Load info;
			info.m_modelName = "yuri.obj";
			info.m_instanceInfo.emplace_back(4.0f, -2.0f, 0.0f);
			info.m_instanceInfo.back().rescale = 2.8f;
			info.m_instanceInfo.back().rotate(glm::radians(180.0f), { 0.0f, 1.0f, 0.0f });
			this->addObject(info);
		}
		
		{
			ModelBuildInfo_Load info;
			info.m_modelName = "honoka.obj";
			info.m_instanceInfo.emplace_back(-4.0f, -2.0f, 0.0f);
			info.m_instanceInfo.back().rescale = 1.2f;
			this->addObject(info);
		}

		{
			ModelBuildInfo_Load info;
			info.m_modelName = "honoka_bunny.obj";
			info.m_instanceInfo.emplace_back(-8.0f, -2.0f, 0.0f);
			info.m_instanceInfo.back().rescale = 1.2f;
			this->addObject(info);
		}

		{
			ModelBuildInfo_Load info;
			info.m_modelName = "honoka_apron.obj";
			info.m_instanceInfo.emplace_back(8.0f, -2.0f, 0.0f);
			info.m_instanceInfo.back().rescale = 3.0f;
			this->addObject(info);
		}

		{
			ModelBuildInfo_Load info;
			info.m_modelName = "honoka_nude.obj";
			info.m_instanceInfo.emplace_back(12.0f, -2.0f, 0.0f);
			info.m_instanceInfo.back().rescale = 3.0f;
			this->addObject(info);
		}

		{
			ModelBuildInfo_Load info;
			info.m_modelName = "brit.obj";
			info.m_instanceInfo.emplace_back(-12.0f, -2.0f, 0.0f);
			info.m_instanceInfo.back().rescale = 3.0f;
			this->addObject(info);
		}

	}

	SceneMaster::~SceneMaster(void) {

	}

	void SceneMaster::notify(iTask* const task) {
		std::unique_ptr<iTask> taskPtr{task};

		if (g_sentTasks_objLoad.find(task) != g_sentTasks_objLoad.end()) {
			g_sentTasks_objLoad.erase(task);
			assert(task->checkNameIs(g_objMtlLoadTask_name));

			auto loaded = reinterpret_cast<ObjMtlLoadTask*>(task);
			if (loaded->out_success != true) {
				LoggerGod::getinst().putError("Failed to load model: "s + loaded->in_objName);
				return;
			}

			ModelInst* model;
			assert(this->findModel(loaded->in_objName.c_str(), &model, nullptr));

			for (auto& obj : loaded->out_infoObj.mObjects) {
				model->m_renderUnits.emplace_back();
				auto& unit = model->m_renderUnits.back();

				unit.m_mesh.buildData(
					obj.mVertices.data(),  obj.mVertices.size(),
					obj.mTexcoords.data(), obj.mTexcoords.size(),
					obj.mNormals.data(),   obj.mNormals.size()
				);
				unit.m_name = obj.mName;

				for (auto& mtl : loaded->out_infoMtl.mMateirals) {
					if (mtl.mName == obj.mMaterialName) {
						unit.m_material.mDiffuseColor = mtl.mDiffuseColor;
						unit.m_material.setName(mtl.mName.c_str());

						if (mtl.mDiffuseMap.size() > 0) {
							auto tex = m_texMas.request_diffuseMap(mtl.mDiffuseMap.c_str());
							unit.m_material.setDiffuseMap(tex);
						}
						break;
					}
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
					glm::mat4 mat;
					inst.getViewMat(&mat);
					glUniformMatrix4fv(uniloc.uModelMat, 1, GL_FALSE, &mat[0][0]);
					unit->m_mesh.draw();
				}
			}
			/*
			for (auto& unit : model.m_renderUnits) {
				unit.m_material.sendUniform(uniloc);
				if (!unit.m_mesh.isReady()) continue;

				for (auto& inst : model.m_inst) {
					glm::mat4 mat;
					inst.getViewMat(&mat);
					glUniformMatrix4fv(uniloc.uModelMat, 1, GL_FALSE, &mat[0][0]);
					unit.m_mesh.draw();
				}
			}
			*/
		}
	}

	void SceneMaster::renderDepthMp(const UnilocDepthmp& uniloc) const {
		for (auto& model : m_freeModels) {
			for (auto& unit : model.m_renderUnits) {
				for (auto& inst : model.m_inst) {
					glm::mat4 mat;
					inst.getViewMat(&mat);
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

		auto task = new ObjMtlLoadTask(info.m_modelName);
		g_sentTasks_objLoad.insert(task);
		TaskGod::getinst().orderTask(task, this);
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