#pragma once
#include "Domain/ClientVersion.h"
#include <filesystem>
#include <map>
#include <memory>
#include <vector>


namespace MapEditor {
namespace Services {

class ConfigService;

/**
 * Manages all supported Tibia client versions.
 * In-memory storage and lookups only (I/O delegated to ClientVersionPersistence).
 */
class ClientVersionRegistry {
public:
  ClientVersionRegistry() = default;
  ~ClientVersionRegistry() = default;

  /**
   * Load client versions from default locations (data/clients.json, etc).
   * Also loads paths from config.
   * @param config Configuration service to load user paths from
   * @return true if successful (even if no clients found but file loaded)
   */
  bool loadDefaults(const ConfigService &config);

  /**
   * Bulk load versions from already-parsed data (from ClientVersionPersistence).
   * @param path Path to clients.json (for saveToFile reference)
   * @param versions Map of version_number -> ClientVersion
   * @param otb_to_version OTB version mapping
   * @param default_version Default version number
   */
  void loadVersions(const std::filesystem::path &json_path,
                    std::map<uint32_t, Domain::ClientVersion> versions,
                    std::map<uint32_t, uint32_t> otb_to_version,
                    uint32_t default_version);

  /**
   * Load saved client paths from config
   */
  void loadPathsFromConfig(const ConfigService &config);

  /**
   * Save client paths to config
   */
  void savePathsToConfig(ConfigService &config) const;

  // Version access
  Domain::ClientVersion *getVersion(uint32_t version_number);
  const Domain::ClientVersion *getVersion(uint32_t version_number) const;

  Domain::ClientVersion *getVersionByOtbVersion(uint32_t otb_version);
  const Domain::ClientVersion *
  getVersionByOtbVersion(uint32_t otb_version) const;

  // Get all versions (sorted by version number descending)
  std::vector<const Domain::ClientVersion *> getAllVersions() const;

  // Get only visible versions (for UI dropdown)
  std::vector<const Domain::ClientVersion *> getVisibleVersions() const;

  // Set client data path for a version
  void setClientPath(uint32_t version_number,
                     const std::filesystem::path &path);

  // Find most suitable version for an OTB minor version
  Domain::ClientVersion *findVersionForOtb(uint32_t otb_minor_version);

  // Check if any version has valid paths
  bool hasAnyValidPaths() const;

  // Set default client version
  void setDefaultVersion(uint32_t version_number);
  uint32_t getDefaultVersion() const { return default_version_; }

  // === CRUD operations ===

  /**
   * Add a new client version.
   * @return true if added, false if version already exists
   */
  bool addClient(const Domain::ClientVersion &version);

  /**
   * Update an existing client version.
   * @return true if updated, false if version not found
   */
  bool updateClient(uint32_t version_number,
                    const Domain::ClientVersion &updated);

  /**
   * Remove a client version.
   * @return true if removed, false if version not found
   */
  bool removeClient(uint32_t version_number);

  /**
   * Get path to clients.json (for external save operations).
   */
  const std::filesystem::path &getJsonPath() const { return clients_json_path_; }

  /**
   * Get all versions data (for external save operations).
   */
  const std::map<uint32_t, Domain::ClientVersion> &getVersionsMap() const {
    return versions_;
  }
  const std::map<uint32_t, uint32_t> &getOtbMapping() const {
    return otb_to_version_;
  }

  // Get count
  size_t getVersionCount() const { return versions_.size(); }
  bool isEmpty() const { return versions_.empty(); }

private:
  std::map<uint32_t, Domain::ClientVersion>
      versions_;                                // version_number -> version
  std::map<uint32_t, uint32_t> otb_to_version_; // otb_version -> version_number
  uint32_t default_version_ = 0;
  std::filesystem::path clients_json_path_; // Path to clients.json for saving
};

} // namespace Services
} // namespace MapEditor
