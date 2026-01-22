#pragma once

#include <memory>

namespace MapEditor {

namespace Services {
    class ConfigService;
    class ClientVersionRegistry;
    class RecentLocationsService;
}

/**
 * Container for core application services.
 * Groups configuration, version registry, and recent files services.
 */
struct ServiceContainer {
    std::unique_ptr<Services::ConfigService> config;
    std::unique_ptr<Services::ClientVersionRegistry> versions;
    std::unique_ptr<Services::RecentLocationsService> recent_locations;
};

} // namespace MapEditor
