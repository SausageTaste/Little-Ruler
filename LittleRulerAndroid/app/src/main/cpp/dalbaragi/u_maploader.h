#pragma once

#include <cstdint>

#include "u_loadinfo.h"


namespace dal {

	bool parseMap_dlb(LoadedMap& info, const uint8_t* const buf, const size_t bufSize);

}