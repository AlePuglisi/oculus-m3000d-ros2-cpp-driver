#include "oculus_app/OculusApp.hpp"

int main(int argc, const char * argv[])
{
    (void)argc;  // Suppress unused parameter warning
    (void)argv;  // Suppress unused parameter warning
    
    OculusApp oculus_app;
    
    // Keep the application running
    std::cout << "Press Ctrl+C to exit..." << std::endl;
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return 0;
}