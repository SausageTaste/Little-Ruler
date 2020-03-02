#pragma once

#include <optional>

#include "d_mapdata.h"


namespace dal {

    std::optional<v1::MapChunkInfo> parseDLB_v1(const uint8_t* const buf, const size_t bufSize);

}
