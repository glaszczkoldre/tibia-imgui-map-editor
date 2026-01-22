#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace MapEditor {
namespace Domain {

/**
 * Represents a supported Tibia client version
 * Contains version info and paths to client data files
 */
class ClientVersion {
public:
  ClientVersion() = default;
  ClientVersion(uint32_t version, const std::string &name,
                uint32_t otb_version);

  // Version identifiers
  uint32_t getVersion() const { return version_; }
  const std::string &getName() const { return name_; }
  uint32_t getOtbVersion() const { return otb_version_; } // otbId - OTBM minor
  uint32_t getOtbMajor() const { return otb_major_; }     // Items major version
  uint32_t getOtbmVersion() const {
    return otbm_version_;
  } // OTBM format version

  void setOtbMajor(uint32_t major) { otb_major_ = major; }
  void setOtbmVersion(uint32_t ver) { otbm_version_ = ver; }

  // File signatures (for validation)
  uint32_t getDatSignature() const { return dat_signature_; }
  uint32_t getSprSignature() const { return spr_signature_; }
  void setDatSignature(uint32_t sig) { dat_signature_ = sig; }
  void setSprSignature(uint32_t sig) { spr_signature_ = sig; }

  // Client data path (user-configured)
  const std::filesystem::path &getClientPath() const { return client_path_; }
  void setClientPath(const std::filesystem::path &path) { client_path_ = path; }

  // Path helpers
  std::filesystem::path getDatPath() const;
  std::filesystem::path getSprPath() const;
  std::filesystem::path getOtbPath() const;

  // Validation
  bool hasValidPaths() const;
  bool validateFiles() const;

  // Feature detection based on version
  bool supportsExtendedSprites() const { return version_ >= 960; }
  bool supportsFrameDurations() const { return version_ >= 1050; }
  bool supportsFrameGroups() const { return version_ >= 1057; }

  // Visibility (some versions are internal/deprecated)
  bool isVisible() const { return visible_; }
  void setVisible(bool visible) { visible_ = visible; }

  // Default client flag
  bool isDefault() const { return is_default_; }
  void setDefault(bool is_default) { is_default_ = is_default; }

  // Data directory and description
  const std::string &getDataDirectory() const { return data_directory_; }
  void setDataDirectory(const std::string &dir) { data_directory_ = dir; }
  const std::string &getDescription() const { return description_; }
  void setDescription(const std::string &desc) { description_ = desc; }

private:
  uint32_t version_ = 0;      // e.g., 860 for 8.60
  std::string name_;          // e.g., "Client 8.60"
  uint32_t otb_version_ = 0;  // OTB minor version (otbId)
  uint32_t otb_major_ = 0;    // OTB major version (items major)
  uint32_t otbm_version_ = 0; // OTBM format version
  uint32_t dat_signature_ = 0;
  uint32_t spr_signature_ = 0;
  std::filesystem::path client_path_;
  std::string data_directory_; // e.g., "740"
  std::string description_;    // User-editable description
  bool visible_ = true;
  bool is_default_ = false;
};

} // namespace Domain
} // namespace MapEditor
