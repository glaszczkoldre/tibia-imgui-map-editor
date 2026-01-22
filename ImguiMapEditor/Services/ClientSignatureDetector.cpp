#include "ClientSignatureDetector.h"
#include <fstream>
#include <spdlog/spdlog.h>

namespace MapEditor {
namespace Services {

uint32_t ClientSignatureDetector::detectFromFolder(
    const std::filesystem::path &folder,
    const std::map<uint32_t, Domain::ClientVersion> &versions) {

  uint32_t dat_sig = readDatSignature(folder);
  uint32_t spr_sig = readSprSignature(folder);

  if (dat_sig == 0 || spr_sig == 0) {
    return 0;
  }

  spdlog::debug("Detected signatures - DAT: 0x{:08X}, SPR: 0x{:08X}", dat_sig,
                spr_sig);

  // Find matching version (both signatures)
  for (const auto &[num, version] : versions) {
    if (version.getDatSignature() == dat_sig &&
        version.getSprSignature() == spr_sig) {
      spdlog::info("Auto-detected client version {} from signatures", num);
      return num;
    }
  }

  // Try matching DAT only (SPR can vary)
  for (const auto &[num, version] : versions) {
    if (version.getDatSignature() == dat_sig) {
      spdlog::info("Auto-detected client version {} from DAT signature only",
                   num);
      return num;
    }
  }

  spdlog::warn("No matching client version found for signatures DAT: 0x{:08X}, "
               "SPR: 0x{:08X}",
               dat_sig, spr_sig);
  return 0;
}

uint32_t
ClientSignatureDetector::readDatSignature(const std::filesystem::path &folder) {
  auto dat_path = folder / "Tibia.dat";

  if (!std::filesystem::exists(dat_path)) {
    return 0;
  }

  uint32_t signature = 0;
  try {
    std::ifstream file(dat_path, std::ios::binary);
    if (file) {
      file.read(reinterpret_cast<char *>(&signature), sizeof(signature));
    }
  } catch (...) {
    return 0;
  }

  return signature;
}

uint32_t
ClientSignatureDetector::readSprSignature(const std::filesystem::path &folder) {
  auto spr_path = folder / "Tibia.spr";

  if (!std::filesystem::exists(spr_path)) {
    return 0;
  }

  uint32_t signature = 0;
  try {
    std::ifstream file(spr_path, std::ios::binary);
    if (file) {
      file.read(reinterpret_cast<char *>(&signature), sizeof(signature));
    }
  } catch (...) {
    return 0;
  }

  return signature;
}

} // namespace Services
} // namespace MapEditor
