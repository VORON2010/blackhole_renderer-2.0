#include "Application.h"
#include <iostream>

int main() {
    Application app;
    if (!app.Init()) {
        std::cerr << "Failed to initialize application.\n";
        return -1;
    }
    app.Run();
    return 0;
}
