#if true

#include <exception>

#include <spdlog/fmt/fmt.h>

#include "x_init_windows.h"

#define DAL_CATCH_EXCEPTION false


#if DAL_CATCH_EXCEPTION
int main(int argc, char* args[]) try {
#else
int main(int argc, char* args[]) {
#endif

    system("chcp 65001");
    return dal::main_windows();
}

#if DAL_CATCH_EXCEPTION
catch ( const std::exception& e ) {
    fmt::print("exception thrown: {}\n", e.what());
}
catch ( ... ) {
    fmt::print("unknown thrown\n");
}
#endif

#else

#include <spdlog/fmt/fmt.h>

#include "u_filesystem.h"


int main(int argc, char* args[]) {
    return 0;
}

#endif
