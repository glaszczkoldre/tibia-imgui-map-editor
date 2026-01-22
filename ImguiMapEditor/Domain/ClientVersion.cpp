#include "ClientVersion.h"
namespace MapEditor {
namespace Domain {

ClientVersion::ClientVersion(uint32_t version, const std::string& name, uint32_t otb_version)
    : version_(version)
    , name_(name)
    , otb_version_(otb_version)
{
}

std::filesystem::path ClientVersion::getDatPath() const {
    if (client_path_.empty()) {
        return {};
    }
    return client_path_ / "Tibia.dat";
}

std::filesystem::path ClientVersion::getSprPath() const {
    if (client_path_.empty()) {
        return {};
    }
    return client_path_ / "Tibia.spr";
}

std::filesystem::path ClientVersion::getOtbPath() const {
    if (client_path_.empty()) {
        return {};
    }
    return client_path_ / "items.otb";
}

bool ClientVersion::hasValidPaths() const {
    if (client_path_.empty()) {
        return false;
    }
    return std::filesystem::exists(client_path_);
}

bool ClientVersion::validateFiles() const {
    if (!hasValidPaths()) {
        return false;
    }
    
    auto dat_path = getDatPath();
    auto spr_path = getSprPath();
    auto otb_path = getOtbPath();
    auto srv_path = client_path_ / "items.srv";
    
    // DAT and SPR are always required
    // For item definitions: accept either items.otb OR items.srv (ancient format)
    return std::filesystem::exists(dat_path) &&
           std::filesystem::exists(spr_path) &&
           (std::filesystem::exists(otb_path) || std::filesystem::exists(srv_path));
}

} // namespace Domain
} // namespace MapEditor
