#include "Application.h"
#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
    // Configure logging
    spdlog::set_level(spdlog::level::info);  // Normal logging level
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    
    spdlog::info("Tibia Map Editor starting...");
    
    MapEditor::Application app;
    
    if (!app.initialize()) {
        spdlog::error("Failed to initialize application");
        return 1;
    }
    
    int result = app.run();
    
    spdlog::info("Tibia Map Editor exiting with code {}", result);
    return result;
}
