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


namespace dal {

	SceneMaster::SceneMaster(void) {
		this->loadMap("asset::maps/test_level.dlb");
	}

	SceneMaster::~SceneMaster(void) {

	}

	void SceneMaster::renderGeneral(const UnilocGeneral& uniloc) const {
		for (auto& map : m_mapChunks) {
			for (auto& modelActor : map.m_actors) {
				modelActor.m_model.renderGeneral(uniloc, modelActor.m_inst);
			}
		}
	}

	void SceneMaster::renderDepthMp(const UnilocDepthmp& uniloc) const {
		for (auto& map : m_mapChunks) {
			for (auto& modelActor : map.m_actors) {
				modelActor.m_model.renderDepthMap(uniloc, modelActor.m_inst);
			}
		}
	}

	void SceneMaster::loadMap(const char* const mapID) {
		ResourceFilePath path;
		parseResFilePath(mapID, path);

		std::vector<uint8_t> buffer;
		auto res = filec::getResource_buffer(mapID, buffer);
		if (!res) throw - 1;

		LoadedMap info;
		info.m_mapName = path.m_name;
		info.m_packageName = path.m_package;

		res = parseMap_dlb(info, buffer.data(), buffer.size());
		if (!res) LoggerGod::getinst().putError("Failed to parse level: "s + mapID);
		this->addMap(info);
	}
		
	// Private

	void SceneMaster::addMap(const LoadedMap& map) {
		this->m_mapChunks.emplace_back();
		auto& newMap = this->m_mapChunks.back();

		for (auto& definedModel : map.m_definedModels) {
			newMap.m_actors.emplace_back();
			auto& modelActor = newMap.m_actors.back();

			modelActor.m_model = this->m_resMas.buildModel(definedModel, map.m_packageName.c_str());

			// Actors
			modelActor.m_inst.assign(definedModel.m_actors.begin(), definedModel.m_actors.end());
		}

		for (auto& importedModel : map.m_importedModels) {
			newMap.m_actors.emplace_back();
			auto& model = newMap.m_actors.back();

			model.m_model = this->m_resMas.orderModel((map.m_packageName + "::" + importedModel.m_modelID).c_str());

			model.m_inst.assign(importedModel.m_actors.begin(), importedModel.m_actors.end());
		}
	}

}