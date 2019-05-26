#include "p_scene.h"

#include <unordered_set>
#include <cassert>

#include "s_logger_god.h"
#include "u_maploader.h"


using namespace std::string_literals;


namespace dal {

	SceneMaster::SceneMaster(ResourceMaster& resMas)
	:	m_resMas(resMas)
	{
		m_mapChunks.emplace_front();
		m_persistantMap = &m_mapChunks.front();
		m_persistantMap->m_name = "persis";

		this->loadMap("asset::map/test_level.dlb");
	}

	SceneMaster::~SceneMaster(void) {

	}

	void SceneMaster::renderGeneral(const UnilocGeneral& uniloc_general) const {
		for (auto& map : m_mapChunks) {
			glUniform1i(uniloc_general.uPlightCount, map.m_plights.size());
			for (unsigned int i = 0; i < map.m_plights.size(); i++) {
				if (i >= 3) break;
				map.m_plights.at(i).sendUniform(uniloc_general, i);
			}

			for (auto& modelActor : map.m_modelActors) {
				modelActor.m_model.renderGeneral(uniloc_general, modelActor.m_inst);
			}
		}
	}

	void SceneMaster::renderDepthMp(const UnilocDepthmp& uniloc) const {
		for (auto& map : m_mapChunks) {
			for (auto& modelActor : map.m_modelActors) {
				modelActor.m_model.renderDepthMap(uniloc, modelActor.m_inst);
			}
		}
	}

	void SceneMaster::renderWaterry(const UnilocWaterry& uniloc) {
		for ( auto& map : m_mapChunks ) {
			glUniform1i(uniloc.uPlightCount, map.m_plights.size());
			for ( unsigned int i = 0; i < map.m_plights.size(); i++ ) {
				if ( i >= 3 ) break;
				map.m_plights.at(i).sendUniform(uniloc, i);
			}

			for ( auto& water : map.m_waters ) {
				water.renderWaterry(uniloc);
			}
		}
	}

	void SceneMaster::renderOnWater(const UnilocGeneral& uniloc, const Camera& cam) {
		for ( auto& map : m_mapChunks ) {
			for ( auto& water : map.m_waters ) {
				{
					glUniform4f(uniloc.u_clipPlane, 0, 1, 0, -water.getHeight() + 0.01f);
					glUniform1i(uniloc.u_doClip, 1);

					auto bansaCam = cam;
					{
						auto camPos = bansaCam.getPos();
						const auto waterHeight = water.getHeight();
						camPos.y = 2 * waterHeight - camPos.y;
						bansaCam.setPos(camPos);

						auto camViewPlane = bansaCam.getViewPlane();
						bansaCam.setViewPlane(camViewPlane.x, -camViewPlane.y);
					}

					const auto viewMat = bansaCam.makeViewMat();
					glUniformMatrix4fv(uniloc.uViewMat, 1, GL_FALSE, &viewMat[0][0]);

					const auto viewPos = bansaCam.getPos();
					glUniform3f(uniloc.uViewPos, viewPos.x, viewPos.y, viewPos.z);

					glUniform3f(uniloc.uBaseAmbient, 0.3f, 0.3f, 0.3f);

					water.m_fbuffer.bindReflectionFrameBuffer();
					glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

					// Render map general
					{
						glUniform1i(uniloc.uPlightCount, map.m_plights.size());
						for ( unsigned int i = 0; i < map.m_plights.size(); i++ ) {
							if ( i >= 3 ) break;
							map.m_plights.at(i).sendUniform(uniloc, i);
						}

						for ( auto& modelActor : map.m_modelActors ) {
							modelActor.m_model.renderGeneral(uniloc, modelActor.m_inst);
						}
					}
				}

				{
					glUniform4f(uniloc.u_clipPlane, 0, -1, 0, water.getHeight());
					glUniform1i(uniloc.u_doClip, 1);

					const auto viewMat = cam.makeViewMat();
					glUniformMatrix4fv(uniloc.uViewMat, 1, GL_FALSE, &viewMat[0][0]);

					const auto viewPos = cam.getPos();
					glUniform3f(uniloc.uViewPos, viewPos.x, viewPos.y, viewPos.z);

					water.m_fbuffer.bindRefractionFrameBuffer();
					glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

					// Render map general
					{
						glUniform1i(uniloc.uPlightCount, map.m_plights.size());
						for ( unsigned int i = 0; i < map.m_plights.size(); i++ ) {
							if ( i >= 3 ) break;
							map.m_plights.at(i).sendUniform(uniloc, i);
						}

						for ( auto& modelActor : map.m_modelActors ) {
							modelActor.m_model.renderGeneral(uniloc, modelActor.m_inst);
						}
					}
				}
			}
		}
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
			for ( auto& water : map.m_waters ) {
				water.m_fbuffer.resizeFbuffer(width, height);
			}
		}
	}

	ActorInfo* SceneMaster::addActorForModel(const ResourceID& resID, const std::string& actorName) {
		auto modelHandle = m_resMas.orderModel(resID);

		for (auto& i : m_persistantMap->m_modelActors) {
			if (i.m_model == modelHandle) {
				i.m_inst.emplace_back();
				return &i.m_inst.back();
			}
		}

		// When not found
		m_persistantMap->m_modelActors.emplace_back();
		auto& newModelActor = m_persistantMap->m_modelActors.back();
		newModelActor.m_model = modelHandle;
		newModelActor.m_inst.emplace_back();
		return &newModelActor.m_inst.back();
	}
		
	// Private

	void SceneMaster::addMap(const LoadedMap& map) {
		this->m_mapChunks.emplace_back();
		auto& newMap = this->m_mapChunks.back();
		newMap.m_name = map.m_mapName;

		for (auto& definedModel : map.m_definedModels) {
			newMap.m_modelActors.emplace_back();
			auto& modelActor = newMap.m_modelActors.back();

			modelActor.m_model = this->m_resMas.buildModel(definedModel, map.m_packageName.c_str());
			modelActor.m_inst.assign(definedModel.m_actors.begin(), definedModel.m_actors.end());
		}

		for (auto& importedModel : map.m_importedModels) {
			newMap.m_modelActors.emplace_back();
			auto& model = newMap.m_modelActors.back();

			ResourceID modelResID{ importedModel.m_modelID };
			if ( modelResID.getPackage().empty() ) modelResID.setPackage(map.m_packageName);

			model.m_model = this->m_resMas.orderModel(modelResID);
			model.m_inst.assign(importedModel.m_actors.begin(), importedModel.m_actors.end());
		}

		for (auto& pointLight : map.m_pointLights) {
            newMap.m_plights.emplace_back();
            auto& plight = newMap.m_plights.back();

            plight.m_name = pointLight.m_name;
            plight.m_color = pointLight.m_color;
            plight.mPos = pointLight.m_pos;
            plight.mMaxDistance = pointLight.m_maxDist;
		}

		for ( auto& waterInfo : map.m_waterPlanes ) {
			glm::vec2 size{ waterInfo.width, waterInfo.height };
			newMap.m_waters.emplace_back(waterInfo.m_pos, size);
		}
	}

}