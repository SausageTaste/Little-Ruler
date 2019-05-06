#include "u_maploader.h"

#include <cstring>
#include <string>

#include "s_logger_god.h"


using namespace std::string_literals;


namespace {

	auto& g_logger = dal::LoggerGod::getinst();

}


namespace {
	namespace typeCodes {

		constexpr int attrib_actor = 3;
		constexpr int item_modelImported = 2;
		constexpr int item_modelDefined = 1;

	}
}


namespace {

	bool isBigEndian() {
		short int number = 0x1;
		char* numPtr = (char*)& number;
		return numPtr[0] != 1;
	}


	int makeInt2(const uint8_t* begin) {
		static_assert(1 == sizeof(uint8_t), "Size of uint8 is not 1 byte. WTF???");
		static_assert(4 == sizeof(float), "Size of float is not 4 bytes.");

		uint8_t buf[4];

		if (isBigEndian()) {
			buf[0] = 0;
			buf[1] = 0;
			buf[2] = begin[1];
			buf[3] = begin[0];
		}
		else {
			buf[0] = begin[0];
			buf[1] = begin[1];
			buf[2] = 0;
			buf[3] = 0;
		}
		
		int res;
		memcpy(&res, buf, 4);
		return res;
	}

	int makeInt4(const uint8_t* begin) {
		static_assert(1 == sizeof(uint8_t), "Size of uint8 is not 1 byte. WTF???");
		static_assert(4 == sizeof(float), "Size of float is not 4 bytes.");

		uint8_t buf[4];

		if (isBigEndian()) {
			buf[0] = begin[3];
			buf[1] = begin[2];
			buf[2] = begin[1];
			buf[3] = begin[0];
		}
		else {
			buf[0] = begin[0];
			buf[1] = begin[1];
			buf[2] = begin[2];
			buf[3] = begin[3];
		}

		int res;
		memcpy(&res, buf, 4);
		return res;
	}

	float makeFloat4(const uint8_t* begin) {
		static_assert(1 == sizeof(uint8_t), "Size of uint8 is not 1 byte. WTF???");
		static_assert(4 == sizeof(float), "Size of float is not 4 bytes.");

		uint8_t buf[4];

		if (isBigEndian()) {
			buf[0] = begin[3];
			buf[1] = begin[2];
			buf[2] = begin[1];
			buf[3] = begin[0];
		}
		else {
			buf[0] = begin[0];
			buf[1] = begin[1];
			buf[2] = begin[2];
			buf[3] = begin[3];
		}

		float res;
		memcpy(&res, buf, 4);
		return res;
	}

}


namespace {  // Make attribs

	const uint8_t* makeAttrib_actor(dal::Actor& info, const uint8_t* const begin, const uint8_t* const end) {
		const uint8_t* header = begin;

		{
			const auto charPtr = reinterpret_cast<const char*>(begin);
			const auto len = std::strlen(charPtr);
			if (len > 512) throw - 1;

			info.m_actorID = charPtr;
			header += len + 1;
		}

		{
			const size_t assumedRestBytes = 4 * (4 + 3);  // (float is 4 bytes) * ( (vec4) + (vec3) )
			if (assumedRestBytes > (end - header)) {
				throw - 1;
			}
		}
		
		{
			float numBuf[7];  // vec3 and vec4 together.

			for (unsigned int i = 0; i < 7; i++) {
				numBuf[i] = makeFloat4(header);
				header += 4;
			}

			printf("Offset %d\n", header - begin);

			info.pos = { numBuf[0], numBuf[1], numBuf[2] };
			info.myQuat.x = numBuf[3];
			info.myQuat.y = numBuf[4];
			info.myQuat.z = numBuf[5];
			info.myQuat.w = numBuf[6];
		}
		
		return header;
	}

}


namespace {  // Make items

	const uint8_t* makeModelImported(dal::LoadedMap& info, const uint8_t* const begin, const uint8_t* const end) {
		const uint8_t* header = begin;
		info.m_importedModels.emplace_back();
		auto& importedModel = info.m_importedModels.back();

		{ // Get model id
			const auto charPtr = reinterpret_cast<const char*>(begin);
			importedModel.m_modelID = charPtr;
			header += std::strlen(charPtr) + 1;
		}

		{ // Get actors
			const auto listElementTypeCode = makeInt2(header);
			assert(typeCodes::attrib_actor == listElementTypeCode);
			header += 2;
			const auto listSize = makeInt4(header);
			header += 4;

			for (int i = 0; i < listSize; i++) {
				importedModel.m_actors.emplace_back();
				auto& actor = importedModel.m_actors.back();
				header = makeAttrib_actor(actor, header, end);
			}
		}

		return header;
	}

	const uint8_t* makeModelDefined(dal::LoadedMap& info, const uint8_t* const begin, const uint8_t* const end) {
		const uint8_t* header = begin;
		info.m_definedModels.emplace_back();
		auto& definedModel = info.m_definedModels.back();

		{ // Get model id
			const auto charPtr = reinterpret_cast<const char*>(header);
			definedModel.m_modelID = charPtr;
			header += std::strlen(charPtr) + 1;
		}

		{ // Get actors
			const auto listElementTypeCode = makeInt2(header); header += 2;
			assert(typeCodes::attrib_actor == listElementTypeCode);
			
			const auto listSize = makeInt4(header); header += 4;
			

			for (int i = 0; i < listSize; i++) {
				definedModel.m_actors.emplace_back();
				auto& actor = definedModel.m_actors.back();
				header = makeAttrib_actor(actor, header, end);
			}
		}

		{ // Get vertex arrays
			std::vector<float>* arrays[3] = {
				&definedModel.m_renderUnit.m_mesh.m_vertices,
				&definedModel.m_renderUnit.m_mesh.m_texcoords,
				&definedModel.m_renderUnit.m_mesh.m_normals
			};

			for (int i = 0; i < 3; i++) {
				const auto arrSize = makeInt4(header); header += 4;

				auto& arr = arrays[i];
				arr->resize(arrSize);
				
				for (int j = 0; j < arrSize; j++) {
					const auto value = makeFloat4(header); header += 4;
					arr->at(j) = value;
				}
			}
		}

		auto& material = definedModel.m_renderUnit.m_material;

		{ // Get diffuse color(3), shininess(1), specular strength(1)
			float floatBuf[5];
			for (int i = 0; i < 5; i++) {
				floatBuf[i] = makeFloat4(header); header += 4;
			}

			material.m_diffuseColor = { floatBuf[0], floatBuf[1], floatBuf[2] };
			material.m_shininess = floatBuf[3];
			material.m_specStrength = floatBuf[4];
		}

		{ // Get diffuse map name
			const auto charPtr = reinterpret_cast<const char*>(header);
			material.m_diffuseMap = charPtr;
			header += std::strlen(charPtr) + 1;
		}

		{ // Get specular map name
			const auto charPtr = reinterpret_cast<const char*>(header);
			material.m_specularMap = charPtr;
			header += std::strlen(charPtr) + 1;
		}

		return end;
	}

	decltype(makeModelImported)* selectMakerFunc(const int typeCode) {

		switch (typeCode) {

		case typeCodes::item_modelDefined:
			return makeModelDefined;
		case typeCodes::item_modelImported:
			return makeModelImported;
		default:
			g_logger.putError("Unknown map item typeCode: "s + std::to_string(typeCode));
			return nullptr;

		}

	}

}


namespace dal {

	bool parseMap_dlb(LoadedMap& info, const uint8_t* const buf, size_t bufSize) {
		const auto end = buf + bufSize;
		const uint8_t* header = buf;

		while (true) {
			const auto typeCode = makeInt2(header);
			auto makerFunc = selectMakerFunc(typeCode);
			if (nullptr == makerFunc) return false;
			header += 2;

			header = makerFunc(info, header, end);
			if (header == end)
				return true;
		}

		return false;
	}

}