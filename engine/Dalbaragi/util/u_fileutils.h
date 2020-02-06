#pragma once

#include <string>
#include <vector>

#include "u_imagebuf.h"


namespace dal {

    bool loadFileText(const char* const respath, std::string& buffer);
    bool loadFileImage(const char* const respath, ImageData& data);
    bool loadFileBuffer(const char* const respath, std::vector<uint8_t>& buffer);

}
