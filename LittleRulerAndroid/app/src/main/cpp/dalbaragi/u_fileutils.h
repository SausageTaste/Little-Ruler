#pragma once

#include "u_loadinfo.h"
#include "u_imagebuf.h"


namespace dal {

    bool loadFileText(const char* const respath, std::string& buffer);
    bool loadFileImage(const char* const respath, ImageFileData& data);
    bool loadFileBuffer(const char* const respath, std::vector<uint8_t>& buffer);

}
