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

	void SceneMaster::renderGeneral(const UnilocGeneral& uniloc) const {
		for (auto& map : m_mapChunks) {
			for (unsigned int i = 0; i < map.m_plights.size(); i++) {
				if (i >= 3) break;
				map.m_plights.at(i).sendUniform(uniloc, i);
			}

			for (auto& modelActor : map.m_modelActors) {
				modelActor.m_model.renderGeneral(uniloc, modelActor.m_inst);
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

	void SceneMaster::loadMap(const ResourceID& mapID) {
		std::vector<uint8_t> buffer;
		auto res = filec::getResource_buffer(mapID, buffer);
		if (!res) throw - 1;

		LoadedMap info;
		info.m_mapName = mapID.getBareName();
		info.m_packageName = mapID.getPackage();

		res = parseMap_dlb(info, buffer.data(), buffer.size());
		if (!res) {
			LoggerGod::getinst().putError("Failed to parse level: "s + mapID.makeIDStr());
		}
		else {
			this->addMap(info);
		}
	}

	Actor* SceneMaster::addActorForModel(const ResourceID& resID, const std::string& actorName) {
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

			model.m_model = this->m_resMas.orderModel((map.m_packageName + "::" + importedModel.m_modelID).c_str());
			model.m_inst.assign(importedModel.m_actors.begin(), importedModel.m_actors.end());
		}

		for (auto& direcLight : map.m_direcLights) {
            newMap.m_dlights.emplace_back();
            auto& dlight = newMap.m_dlights.back();

            dlight.m_name = direcLight.m_name;
            dlight.m_color = direcLight.m_color;
            dlight.mDirection = direcLight.m_direction;
            dlight.mHalfShadowEdgeSize = direcLight.m_halfShadowEdgeSize;
		}

		for (auto& pointLight : map.m_pointLights) {
            newMap.m_plights.emplace_back();
            auto& plight = newMap.m_plights.back();

            plight.m_name = pointLight.m_name;
            plight.m_color = pointLight.m_color;
            plight.mPos = pointLight.m_pos;
            plight.mMaxDistance = pointLight.m_maxDist;
		}
	}

}