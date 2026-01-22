#pragma once
#include "Domain/ClientVersion.h"
#include <filesystem>
#include <map>

namespace MapEditor {
namespace Services {

/**
 * Detects client version from DAT/SPR file signatures.
 */
class ClientSignatureDetector {
public:
  /**
   * Detect client version by reading DAT/SPR signatures from a folder.
   * @param folder Path to folder containing Tibia.dat and Tibia.spr
   * @param versions Map of known versions to match against
   * @return Detected version number, or 0 if not detected
   */
  static uint32_t
  detectFromFolder(const std::filesystem::path &folder,
                   const std::map<uint32_t, Domain::ClientVersion> &versions);

  /**
   * Read DAT file signature from a folder.
   * @return Signature value, or 0 if file not found
   */
  static uint32_t readDatSignature(const std::filesystem::path &folder);

  /**
   * Read SPR file signature from a folder.
   * @return Signature value, or 0 if file not found
   */
  static uint32_t readSprSignature(const std::filesystem::path &folder);
};

} // namespace Services
} // namespace MapEditor
