// icp.cpp 
// author: JJ

#include <iostream>

#include "app.hpp"

#ifdef _WIN32
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;

    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

// define our application
App app;

int main()
{
    try {
        if (app.init())
            return app.run();
    }
    catch (std::exception const& e) {
        std::cerr << "App failed : " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}
