#if true

#include "x_init_windows.h"


int main(int argc, char* args[]) {
    return dal::main_windows();
}

#else

#include <set>
#include <string>
#include <iostream>

int main(int argc, char* args[]) {
    std::set<std::string> s{ "fuck", "shit", "jackass", "ass", "damn" };

    for ( auto& e : s ) {
        std::cout << e << ", ";
    }
    std::cout << std::endl;
    
    return 0;
}

#endif