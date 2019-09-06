#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>

#include <glm/glm.hpp>

#include "m_collider.h"
#include "g_actor.h"
#include "u_loadinfo.h"


namespace dal {

    std::optional<dlb::MapChunkInfo> parseDLB(const uint8_t* const buf, const size_t bufSize);

}