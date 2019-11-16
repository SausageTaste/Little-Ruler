#if true

#include "x_init_windows.h"


int main(int argc, char* args[]) {
    return dal::main_windows();
}

#else

#include <fmt/format.h>

#include "u_filesystem.h"


int main(int argc, char* args[]) {
    return 0;
}

#endif
