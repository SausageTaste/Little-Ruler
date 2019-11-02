#if true

#include "x_init_windows.h"


int main(int argc, char* args[]) {
    return dal::main_windows();
}

#else

#include "u_objparser.h"


int main(int argc, char* args[]) {
    dal::AssimpModelInfo dae, dmd;

    const auto result1 = dal::loadAssimpModel("asset::Character Running.dae", dae);
    const auto result2 = dal::loadDalModel("asset::Character Running.dmd", dmd);

    return 0;
}

#endif
