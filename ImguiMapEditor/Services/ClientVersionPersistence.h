#pragma once
#include "Domain/ClientVersion.h"
#include <filesystem>
#include <map>

namespace MapEditor {
namespace Services {

struct ClientVersionsData {
  std::map<uint32_t, Domain::ClientVersion> versions;
  std::map<uint32_t, uint32_t> otb_to_version;
  uint32_t default_version = 0;
};

/**
 * Handles JSON serialization/deserialization for client versions.
 * Reads from and writes to clients.json.
 */
class ClientVersionPersistence {
public:
  /**
   * Load client versions from clients.json file.
   * @param path Path to clients.json
   * @return Loaded data, or empty on failure
   */
  static ClientVersionsData loadFromJson(const std::filesystem::path &path);

  /**
   * Save client versions to clients.json file.
   * @param path Path to clients.json
   * @param data Client versions data to save
   * @return true if saved successfully
   */
  static bool saveToJson(const std::filesystem::path &path,
                         const ClientVersionsData &data);
};

} // namespace Services
} // namespace MapEditor
