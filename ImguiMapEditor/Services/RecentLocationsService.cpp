#include "RecentLocationsService.h"
#include "ConfigService.h"
#include <algorithm>

namespace MapEditor {
namespace Services {

void RecentLocationsService::addRecentMap(const std::filesystem::path& path, uint32_t version) {
    // Remove existing entry for same path
    recent_maps_.erase(
        std::remove_if(recent_maps_.begin(), recent_maps_.end(),
            [&path](const RecentEntry& e) { return e.path == path; }),
        recent_maps_.end());
    
    // Add new entry at front
    RecentEntry entry;
    entry.path = path;
    entry.client_version = version;
    entry.last_used = std::chrono::system_clock::now();
    recent_maps_.insert(recent_maps_.begin(), entry);
    
    // Trim to max size
    if (recent_maps_.size() > MAX_RECENT_MAPS) {
        recent_maps_.resize(MAX_RECENT_MAPS);
    }
}

void RecentLocationsService::addRecentClient(const std::filesystem::path& path, uint32_t version) {
    // Remove existing entry for same path
    recent_clients_.erase(
        std::remove_if(recent_clients_.begin(), recent_clients_.end(),
            [&path](const RecentEntry& e) { return e.path == path; }),
        recent_clients_.end());
    
    // Add new entry at front
    RecentEntry entry;
    entry.path = path;
    entry.client_version = version;
    entry.last_used = std::chrono::system_clock::now();
    recent_clients_.insert(recent_clients_.begin(), entry);
    
    // Trim to max size
    if (recent_clients_.size() > MAX_RECENT_CLIENTS) {
        recent_clients_.resize(MAX_RECENT_CLIENTS);
    }
}

void RecentLocationsService::loadFromConfig(const ConfigService& config) {
    recent_maps_.clear();
    recent_clients_.clear();
    
    // Load recent maps
    auto maps_json = config.get<std::vector<nlohmann::json>>("recent_maps", {});
    for (const auto& entry : maps_json) {
        if (entry.contains("path") && entry.contains("version")) {
            RecentEntry e;
            e.path = entry["path"].get<std::string>();
            e.client_version = entry["version"].get<uint32_t>();
            if (entry.contains("timestamp")) {
                e.last_used = std::chrono::system_clock::from_time_t(
                    entry["timestamp"].get<int64_t>());
            } else {
                e.last_used = std::chrono::system_clock::now();
            }
            recent_maps_.push_back(e);
        }
    }
    
    // Load recent clients
    auto clients_json = config.get<std::vector<nlohmann::json>>("recent_clients", {});
    for (const auto& entry : clients_json) {
        if (entry.contains("path") && entry.contains("version")) {
            RecentEntry e;
            e.path = entry["path"].get<std::string>();
            e.client_version = entry["version"].get<uint32_t>();
            if (entry.contains("timestamp")) {
                e.last_used = std::chrono::system_clock::from_time_t(
                    entry["timestamp"].get<int64_t>());
            } else {
                e.last_used = std::chrono::system_clock::now();
            }
            recent_clients_.push_back(e);
        }
    }
    
    // Load default client
    default_client_version_ = config.get<uint32_t>("default_client_version", 0);
}

void RecentLocationsService::saveToConfig(ConfigService& config) const {
    // Save recent maps
    std::vector<nlohmann::json> maps_json;
    for (const auto& e : recent_maps_) {
        nlohmann::json entry;
        entry["path"] = e.path.string();
        entry["version"] = e.client_version;
        entry["timestamp"] = std::chrono::system_clock::to_time_t(e.last_used);
        maps_json.push_back(entry);
    }
    config.set("recent_maps", maps_json);
    
    // Save recent clients
    std::vector<nlohmann::json> clients_json;
    for (const auto& e : recent_clients_) {
        nlohmann::json entry;
        entry["path"] = e.path.string();
        entry["version"] = e.client_version;
        entry["timestamp"] = std::chrono::system_clock::to_time_t(e.last_used);
        clients_json.push_back(entry);
    }
    config.set("recent_clients", clients_json);
    
    // Save default client
    config.set("default_client_version", default_client_version_);
}

} // namespace Services
} // namespace MapEditor
