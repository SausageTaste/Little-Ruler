#pragma once

#include <vector>
#include <string>


namespace dal {

    std::vector<std::string> listdir(const char* const resPath);
    std::vector<std::string> listfile(const char* const resPath);
    std::vector<std::string> listfolder(const char* const resPath);

    bool isdir(const char* const resPath);
    bool isfile(const char* const resPath);
    bool isfolder(const char* const resPath);

}
