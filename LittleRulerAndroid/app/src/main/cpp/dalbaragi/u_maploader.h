#pragma once

#include <cstdint>

#include "u_loadinfo.h"


namespace dal {

	bool parseMap_dlb(LoadedMap& info, const uint8_t* const buf, size_t bufSize);

}