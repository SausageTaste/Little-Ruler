#if true

#include <exception>

#include <fmt/format.h>

#include "x_init_windows.h"


int main(int argc, char* args[]) try {
    system("chcp 65001");
    return dal::main_windows();
}
catch ( const std::exception& e ) {
    fmt::print("exception thrown: {}\n", e.what());
}
catch ( ... ) {
    fmt::print("unknown thrown\n");
}

#else

#include <fmt/format.h>

#include "u_filesystem.h"


int main(int argc, char* args[]) {
    return 0;
}

#endif
