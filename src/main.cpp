#include "engine/Application.h"
#include <iostream>

int main(int /*argc*/, char* /*argv*/[]) {
    try {
        Application app;
        if (!app.init("Tonka Construction", 640, 480)) {
            std::cerr << "Failed to initialize application\n";
            return 1;
        }
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
