#if false

#include "x_init_windows.h"


int main(int argc, char* args[]) {
    return dal::main_windows();
}

#else

#include <fmt/format.h>

#include "u_filesystem.h"


int main(int argc, char* args[]) {
    for ( auto& x : dal::listfolder("asset/texture") ) {
        const auto filepath = fmt::format("asset/texture/{}", x);
        const auto isFile = dal::isfile(filepath.c_str());
        const auto isFolder = dal::isfolder(filepath.c_str());
        const auto isDir = dal::isdir(filepath.c_str());
        fmt::print("{}, {}, {}, {}\n", isFile, isFolder, isDir, x);
    }

    fmt::print("\n\n");

    for ( auto& x : dal::listfile("asset/texture") ) {
        const auto filepath = fmt::format("asset/texture/{}", x);
        const auto isFile = dal::isfile(filepath.c_str());
        const auto isFolder = dal::isfolder(filepath.c_str());
        const auto isDir = dal::isdir(filepath.c_str());
        fmt::print("{}, {}, {}, {}\n", isFile, isFolder, isDir, x);
    }

    return 0;
}

#endif
