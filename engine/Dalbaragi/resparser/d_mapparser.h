#pragma once

#include <optional>

#include "d_mapdata.h"


namespace dal {

    std::optional<v1::LevelData> parseLevel_v1(const uint8_t* const buf, const size_t bufSize);

    std::optional<v1::MapChunk> parseMapChunk_v1(const uint8_t* const buf, const size_t bufSize);

}
