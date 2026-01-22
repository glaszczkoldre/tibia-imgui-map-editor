#pragma once
#include <cstdint>
#include <filesystem>
#include <string>


namespace MapEditor {
namespace UI {

/**
 * Compatibility check result for second map loading.
 */
struct MapCompatibilityResult {
  bool compatible = true;

  // Map info (from OTBM header)
  uint32_t map_items_major = 0;
  uint32_t map_items_minor = 0;
  std::string map_name;

  // Current client info
  uint32_t client_items_major = 0;
  uint32_t client_items_minor = 0;
  uint32_t client_version = 0;

  std::string error_message;
};

/**
 * Modal popup shown when loading an incompatible second map.
 *
 * Provides 3 options:
 * - Cancel: Don't load the map
 * - Force Load: Load with current client (may display incorrectly)
 * - Load with New Client: Placeholder for future implementation
 */
class MapCompatibilityPopup {
public:
  enum class Result { None, Cancel, ForceLoad, LoadWithNewClient };

  MapCompatibilityPopup() = default;

  void show(const MapCompatibilityResult &compat,
            const std::filesystem::path &map_path);
  void render();

  bool isOpen() const { return is_open_; }
  bool hasResult() const { return result_ != Result::None; }
  Result consumeResult();

  const std::filesystem::path &getMapPath() const { return map_path_; }

private:
  bool is_open_ = false;
  Result result_ = Result::None;
  MapCompatibilityResult compat_info_;
  std::filesystem::path map_path_;
};

} // namespace UI
} // namespace MapEditor
