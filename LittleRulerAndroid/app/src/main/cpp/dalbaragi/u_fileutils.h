#pragma once

#include "u_loadinfo.h"


namespace dal {

    bool loadFileText(const char* const respath, std::string& buffer);
    bool loadFileImage(const char* const respath, binfo::ImageFileData& data);
    bool loadFileBuffer(const char* const respath, std::vector<uint8_t>& buffer);

}
