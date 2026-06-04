#pragma once
#include "Domain/ClientVersion.h"
#include <filesystem>
#include <map>

namespace MapEditor {
namespace Services {

class ClientSignatureDetector {
public:
  static uint32_t detectFromFolder(
      const std::filesystem::path &folder,
      const std::map<uint32_t, Domain::ClientVersion> &versions);

  static uint32_t readDatSignature(const std::filesystem::path &folder);

  static uint32_t readSprSignature(const std::filesystem::path &folder);
};

} // namespace Services
} // namespace MapEditor
