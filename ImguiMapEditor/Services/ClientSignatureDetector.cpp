#include "ClientSignatureDetector.h"
#include "ClientAssetDetector.h"
#include <fstream>
#include <spdlog/spdlog.h>

namespace MapEditor {
namespace Services {

uint32_t ClientSignatureDetector::detectFromFolder(
    const std::filesystem::path &folder,
    const std::map<uint32_t, Domain::ClientVersion> &versions) {
  return ClientAssetDetector::detectVersion(folder, versions);
}

uint32_t ClientSignatureDetector::readDatSignature(const std::filesystem::path &folder) {
  auto dat_path = folder / "Tibia.dat";
  if (!std::filesystem::exists(dat_path))
    return 0;

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

uint32_t ClientSignatureDetector::readSprSignature(const std::filesystem::path &folder) {
  auto spr_path = folder / "Tibia.spr";
  if (!std::filesystem::exists(spr_path))
    return 0;

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
