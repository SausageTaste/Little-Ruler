#include "p_scene.h"

#include <unordered_set>
#include <cassert>

#include "s_logger_god.h"
#include "u_maploader.h"


using namespace std::string_literals;


namespace {

	dal::Camera makeReflectionCamera(dal::Camera camera, const float waterHeight) {
		auto camPos = camera.getPos();
		camPos.y = 2.0f * waterHeight - camPos.y;
		camera.setPos(camPos);

		auto camViewPlane = camera.getViewPlane();
		camera.setViewPlane(camViewPlane.x, -camViewPlane.y);

		return camera;
	}

}


namespace dal {

	MapChunk::MapChunk(const std::string& name)
		: m_name(name)
	{

	}

	MapChunk::MapChunk(const LoadedMap& info, ResourceMaster& resMas)
		: m_name(info.m_mapName)
	{
		for ( auto& definedModel : info.m_definedModels ) {
			this->m_modelActors.emplace_back();
			auto& modelActor = this->m_modelActors.back();

			modelActor.m_model = resMas.buildModel(definedModel, info.m_packageName.c_str());
			modelActor.m_inst.assign(definedModel.m_actors.begin(), definedModel.m_actors.end());
		}

		for ( auto& importedModel : info.m_importedModels ) {
			this->m_modelActors.emplace_back();
			auto& model = this->m_modelActors.back();

			ResourceID modelResID{ importedModel.m_modelID };
			if ( modelResID.getPackage().empty() ) modelResID.setPackage(info.m_packageName);

			model.m_model = resMas.orderModel(modelResID);
			model.m_inst.assign(importedModel.m_actors.begin(), importedModel.m_actors.end());
		}

		for ( auto& pointLight : info.m_pointLights ) {
			this->m_plights.emplace_back();
			auto& plight = this->m_plights.back();

			plight.m_name = pointLight.m_name;
			plight.m_color = pointLight.m_color;
			plight.mPos = pointLight.m_pos;
			plight.mMaxDistance = pointLight.m_maxDist;
		}

		for ( auto& waterInfo : info.m_waterPlanes ) {
			glm::vec2 size{ waterInfo.width, waterInfo.height };
			this->m_waters.emplace_back(waterInfo.m_pos, size);
		}
	}

	const std::string& MapChunk::getName(void) const {
		return this->m_name;
	}

	void MapChunk::onScreanResize(const unsigned int width, const unsigned int height) {
		for ( auto& water : this->m_waters ) {
			water.m_fbuffer.resizeFbuffer(width, height);
		}
	}


	void MapChunk::renderGeneral(const UnilocGeneral& uniloc) const {
		glUniform1i(uniloc.uPlightCount, this->m_plights.size());
		for ( size_t i = 0; i < this->m_plights.size(); i++ ) {
			if ( i >= 3 ) break;
			this->m_plights.at(i).sendUniform(uniloc, i);
		}

		for ( auto& modelActor : this->m_modelActors ) {
			modelActor.m_model->renderGeneral(uniloc, modelActor.m_inst);
		}
	}

	void MapChunk::renderDepthMp(const UnilocDepthmp& uniloc) const {
		for ( auto& modelActor : this->m_modelActors ) {
			modelActor.m_model->renderDepthMap(uniloc, modelActor.m_inst);
		}
	}

	void MapChunk::renderWaterry(const UnilocWaterry& uniloc) {
		glUniform1i(uniloc.uPlightCount, this->m_plights.size());
		for ( unsigned int i = 0; i < this->m_plights.size(); i++ ) {
			if ( i >= 3 ) break;
			this->m_plights.at(i).sendUniform(uniloc, i);
		}

		for ( auto& water : this->m_waters ) {
			water.renderWaterry(uniloc);
		}
	}

	void MapChunk::renderGeneral_onWater(const UnilocGeneral& uniloc, const Camera& cam) {
		for ( auto& water : this->m_waters ) {
			{
				// Uniform values

				glUniform4f(uniloc.u_clipPlane, 0, 1, 0, -water.getHeight() + 0.01f);
				glUniform1i(uniloc.u_doClip, 1);

				const auto reflectionCam = makeReflectionCamera(cam, water.getHeight());

				const auto viewMat = reflectionCam.makeViewMat();
				glUniformMatrix4fv(uniloc.uViewMat, 1, GL_FALSE, &viewMat[0][0]);

				const auto viewPos = reflectionCam.getPos();
				glUniform3f(uniloc.uViewPos, viewPos.x, viewPos.y, viewPos.z);

				water.m_fbuffer.bindReflectionFrameBuffer();
				glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

				// Render map general
				this->renderGeneral(uniloc);
			}

			{
				// Uniform values

				glUniform4f(uniloc.u_clipPlane, 0, -1, 0, water.getHeight());
				glUniform1i(uniloc.u_doClip, 1);

				const auto viewMat = cam.makeViewMat();
				glUniformMatrix4fv(uniloc.uViewMat, 1, GL_FALSE, &viewMat[0][0]);

				const auto viewPos = cam.getPos();
				glUniform3f(uniloc.uViewPos, viewPos.x, viewPos.y, viewPos.z);

				water.m_fbuffer.bindRefractionFrameBuffer();
				glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

				// Render map general
				this->renderGeneral(uniloc);
			}
		}
	}


	WaterRenderer* MapChunk::getWater(const int index) {
		return &this->m_waters.at(index);
	}

}


namespace dal {

	SceneMaster::SceneMaster(ResourceMaster& resMas)
	:	m_resMas(resMas)
	{
		m_mapChunks.emplace_front("persis");
		m_persistantMap = &m_mapChunks.front();

		this->loadMap("asset::map/test_level.dlb");
	}

	SceneMaster::~SceneMaster(void) {

	}


	void SceneMaster::renderGeneral(const UnilocGeneral& uniloc) const {
		for (auto& map : m_mapChunks) {
			map.renderGeneral(uniloc);
		}
	}

	void SceneMaster::renderDepthMp(const UnilocDepthmp& uniloc) const {
		for (auto& map : m_mapChunks) {
			map.renderDepthMp(uniloc);
		}
	}

	void SceneMaster::renderWaterry(const UnilocWaterry& uniloc) {
		for ( auto& map : m_mapChunks ) {
			map.renderWaterry(uniloc);
		}
	}

	void SceneMaster::renderGeneral_onWater(const UnilocGeneral& uniloc, const Camera& cam) {
		for ( auto& map : m_mapChunks ) {
			map.renderGeneral_onWater(uniloc, cam);
		}
	}


	WaterRenderer* SceneMaster::getWater(const std::string& mapName, const int index) {
		for ( auto& map : this->m_mapChunks ) {
			if ( mapName == map.getName() ) {
				return map.getWater(index);
			}
		}

		return nullptr;
	}


	void SceneMaster::loadMap(const ResourceID& mapID) {
		std::vector<uint8_t> buffer;
		auto res = futil::getRes_buffer(mapID, buffer);
		if (!res) dalAbort("Failed to load map file: "s + mapID.makeIDStr());

		LoadedMap info;
		info.m_mapName = mapID.getBareName();
		info.m_packageName = mapID.getPackage();

		res = parseMap_dlb(info, buffer.data(), buffer.size());
		if (!res) {
			LoggerGod::getinst().putError("Failed to parse level: "s + mapID.makeIDStr(), __LINE__, __func__, __FILE__);
		}
		else {
			this->addMap(info);
		}
	}

	void SceneMaster::onResize(const unsigned int width, const unsigned int height) {
		for ( auto& map : this->m_mapChunks ) {
			map.onScreanResize(width, height);
		}
	}

	// Private

	void SceneMaster::addMap(const LoadedMap& map) {
		this->m_mapChunks.emplace_back(map, this->m_resMas);
		dalInfo("Map added: "s + this->m_mapChunks.back().getName());
	}

}