#include "SecondaryClientData.h"
#include "ClientSignatureDetector.h"
#include "ClientVersionRegistry.h"
#include "ConfigService.h"
#include "ItemDefinitionResolver.h"
#include "IO/OtbReader.h"
#include "IO/Readers/DatReaderFactory.h"
#include <spdlog/spdlog.h>

namespace MapEditor::Services {

SecondaryClientResult SecondaryClientData::loadFromFolder(
    const std::filesystem::path& folder_path,
    const ClientVersionRegistry& registry)
{
    SecondaryClientResult result;
    
    // Clear any previous data
    clear();
    
    spdlog::info("SecondaryClientData: Loading from folder {}", folder_path.string());
    
    // 1. Auto-find required files
    auto dat_path = folder_path / "Tibia.dat";
    auto spr_path = folder_path / "Tibia.spr";
    auto otb_path = folder_path / "items.otb";
    
    if (!std::filesystem::exists(dat_path)) {
        result.error = "Tibia.dat not found in folder";
        spdlog::error("{}", result.error);
        return result;
    }
    
    if (!std::filesystem::exists(spr_path)) {
        result.error = "Tibia.spr not found in folder";
        spdlog::error("{}", result.error);
        return result;
    }
    
    if (!std::filesystem::exists(otb_path)) {
        result.error = "items.otb not found in folder";
        spdlog::error("{}", result.error);
        return result;
    }
    
    // 2. Auto-detect client version from DAT/SPR signatures
    uint32_t detected_version = ClientSignatureDetector::detectFromFolder(
        folder_path, registry.getVersionsMap());
    if (detected_version == 0) {
        result.error = "Could not detect client version from DAT/SPR signatures";
        spdlog::error("{}", result.error);
        return result;
    }
    
    result.client_version = detected_version;
    spdlog::info("SecondaryClientData: Auto-detected version v{}", detected_version);
    
    // 3. Load items.otb
    spdlog::debug("SecondaryClientData: Loading {}", otb_path.string());
    auto otb_result = IO::OtbReader::read(otb_path);
    if (!otb_result.success) {
        result.error = "Failed to load OTB: " + otb_result.error;
        spdlog::error("{}", result.error);
        return result;
    }
    
    // 4. Load Tibia.dat
    spdlog::debug("SecondaryClientData: Loading {}", dat_path.string());
    auto dat_result = IO::DatReaderFactory::read(dat_path, detected_version);
    if (!dat_result.success) {
        result.error = "Failed to load DAT: " + dat_result.error;
        spdlog::error("{}", result.error);
        return result;
    }
    
    // 5. Load Tibia.spr
    // Extended sprite format (32-bit IDs) required for Tibia 9.60+
    bool uses_extended_sprites = (detected_version >= 960);
    spdlog::debug("SecondaryClientData: Loading {} (extended={})", spr_path.string(), uses_extended_sprites);
    spr_reader_ = std::make_unique<IO::SprReader>();
    auto spr_result = spr_reader_->open(spr_path, 0, uses_extended_sprites);
    if (!spr_result.success) {
        result.error = "Failed to load SPR: " + spr_result.error;
        spdlog::error("{}", result.error);
        spr_reader_.reset();
        return result;
    }
    result.sprite_count = static_cast<size_t>(spr_reader_->getSpriteCount());
    
    spdlog::info("SecondaryClientData: SPR reader reports {} sprites (extended={})", 
                 result.sprite_count, uses_extended_sprites);
    
    // 6. Resolve OTB/DAT into the same final item definitions as the primary client.
    items_ = ItemDefinitionResolver::resolve(Domain::ItemDataSource::OTB,
                                             otb_result.items, dat_result);
    for (size_t index = 0; index < items_.size(); ++index) {
        if (items_[index].server_id > 0) {
            server_id_index_[items_[index].server_id] = index;
        }
    }
    
    result.item_count = items_.size();
    client_version_ = detected_version;
    folder_path_ = folder_path;
    loaded_ = true;
    active_ = true;  // Activate by default
    result.success = true;
    
    spdlog::info("SecondaryClientData: Loaded {} items, {} sprites from v{}",
                 result.item_count, result.sprite_count, detected_version);
    
    return result;
}

const Domain::ItemType* SecondaryClientData::getItemTypeByServerId(uint16_t server_id) const {
    auto it = server_id_index_.find(server_id);
    if (it == server_id_index_.end()) {
        return nullptr;
    }
    return &items_[it->second];
}

void SecondaryClientData::loadSettingsFromConfig(const ConfigService& config) {
    tint_intensity_ = config.get<float>("secondary.tint_intensity", 0.7f);
    alpha_multiplier_ = config.get<float>("secondary.alpha_multiplier", 1.0f);
}

void SecondaryClientData::saveSettingsToConfig(ConfigService& config) const {
    config.set("secondary.tint_intensity", tint_intensity_);
    config.set("secondary.alpha_multiplier", alpha_multiplier_);
}

void SecondaryClientData::clear() {
    loaded_ = false;
    active_ = false;
    client_version_ = 0;
    folder_path_.clear();
    items_.clear();
    server_id_index_.clear();
    spr_reader_.reset();
    spdlog::debug("SecondaryClientData: Cleared");
}

} // namespace MapEditor::Services


