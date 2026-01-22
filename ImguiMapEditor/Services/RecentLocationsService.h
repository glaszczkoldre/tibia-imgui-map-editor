#pragma once

#include <vector>
#include <filesystem>
#include <chrono>
#include <cstdint>
#include "Core/Config.h"

namespace MapEditor {
namespace Services {

class ConfigService;

/**
 * Entry for a recently used map or client
 */
struct RecentEntry {
    std::filesystem::path path;
    uint32_t client_version = 0;
    std::chrono::system_clock::time_point last_used;
    
    // For sorting by most recent
    bool operator<(const RecentEntry& other) const {
        return last_used > other.last_used;
    }
};

/**
 * Service for tracking recent maps, recent clients, and default client
 */
class RecentLocationsService {
public:
    RecentLocationsService() = default;
    
    // Recent maps
    void addRecentMap(const std::filesystem::path& path, uint32_t version);
    const std::vector<RecentEntry>& getRecentMaps() const { return recent_maps_; }
    void clearRecentMaps() { recent_maps_.clear(); }
    
    // Recent clients
    void addRecentClient(const std::filesystem::path& path, uint32_t version);
    const std::vector<RecentEntry>& getRecentClients() const { return recent_clients_; }
    void clearRecentClients() { recent_clients_.clear(); }
    
    // Default client
    void setDefaultClientVersion(uint32_t version) { default_client_version_ = version; }
    uint32_t getDefaultClientVersion() const { return default_client_version_; }
    
    // Persistence
    void loadFromConfig(const ConfigService& config);
    void saveToConfig(ConfigService& config) const;
    
private:
    std::vector<RecentEntry> recent_maps_;
    std::vector<RecentEntry> recent_clients_;
    uint32_t default_client_version_ = 0;
    
    static constexpr size_t MAX_RECENT_MAPS = Config::Data::MAX_RECENT_MAPS;
    static constexpr size_t MAX_RECENT_CLIENTS = Config::Data::MAX_RECENT_CLIENTS;
};

} // namespace Services
} // namespace MapEditor
