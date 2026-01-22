#pragma once
#include "Domain/ItemType.h"
#include "IO/SprReader.h"
#include "SecondaryClientConstants.h"
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <algorithm>

namespace MapEditor::Services {

class ClientVersionRegistry;
class ConfigService;

/**
 * Result of loading a secondary client.
 */
struct SecondaryClientResult {
    bool success = false;
    std::string error;
    uint32_t client_version = 0;
    size_t item_count = 0;
    size_t sprite_count = 0;
};

/**
 * Manages secondary client data for cross-version item visualization.
 * 
 * When the primary client doesn't have an item definition (by server ID),
 * TileRenderer can fall back to this secondary client to:
 * - Get ItemType properties (ground, border, stacking order, etc.)
 * - Load sprites (with SECONDARY_SPRITE_OFFSET applied)
 * 
 * This enables proper OTBM parsing even when the primary client is older.
 * Items from secondary client render with a red tint for visual distinction.
 */
class SecondaryClientData {
public:
    SecondaryClientData() = default;
    ~SecondaryClientData() = default;
    
    // Non-copyable
    SecondaryClientData(const SecondaryClientData&) = delete;
    SecondaryClientData& operator=(const SecondaryClientData&) = delete;
    
    /**
     * Load secondary client from a folder containing all files.
     * Auto-detects: Tibia.dat, Tibia.spr, items.otb
     * Auto-detects client version from DAT/SPR signatures.
     * @param folder_path Path containing Tibia.dat, Tibia.spr, and items.otb
     * @param registry Version registry for version detection
     * @return Result with status and statistics
     */
    SecondaryClientResult loadFromFolder(
        const std::filesystem::path& folder_path,
        const ClientVersionRegistry& registry);
    
    /**
     * Check if secondary client is loaded.
     */
    bool isLoaded() const { return loaded_; }
    
    /**
     * Check if secondary client rendering is active.
     */
    bool isActive() const { return loaded_ && active_; }
    
    /**
     * Set active state (enables/disables secondary rendering).
     */
    void setActive(bool active) { active_ = active; }
    
    /**
     * Get/Set tint intensity for secondary items (0.0 = no tint, 1.0 = full red).
     * Default 0.7 for strong red tint visibility.
     */
    float getTintIntensity() const { return tint_intensity_; }
    void setTintIntensity(float intensity) { tint_intensity_ = std::clamp(intensity, 0.0f, 1.0f); }
    
    /**
     * Get/Set alpha multiplier for secondary items (0.3 = very transparent, 1.0 = opaque).
     * Default 1.0 for full opacity.
     */
    float getAlphaMultiplier() const { return alpha_multiplier_; }
    void setAlphaMultiplier(float alpha) { alpha_multiplier_ = std::clamp(alpha, 0.3f, 1.0f); }
    
    /**
     * Get client version.
     */
    uint32_t getClientVersion() const { return client_version_; }
    
    /**
     * Get loaded folder path.
     */
    const std::filesystem::path& getFolderPath() const { return folder_path_; }
    
    /**
     * Look up ItemType by server ID.
     * @return nullptr if not found in secondary client
     */
    const Domain::ItemType* getItemTypeByServerId(uint16_t server_id) const;
    
    /**
     * Get sprite reader for loading secondary sprites.
     * SpriteManager uses this to load sprites with offset IDs.
     */
    IO::SprReader* getSpriteReader() { return spr_reader_.get(); }
    const IO::SprReader* getSpriteReader() const { return spr_reader_.get(); }
    
    /**
     * Get count of loaded items.
     */
    size_t getItemCount() const { return items_.size(); }
    
    /**
     * Load visual settings from config (tint, alpha).
     */
    void loadSettingsFromConfig(const ConfigService& config);
    
    /**
     * Save visual settings to config.
     */
    void saveSettingsToConfig(ConfigService& config) const;
    
    /**
     * Clear all loaded data (release memory).
     */
    void clear();

private:
    bool loaded_ = false;
    bool active_ = true;  // Active by default when loaded
    float tint_intensity_ = 0.7f;   // Strong red tint by default
    float alpha_multiplier_ = 1.0f; // Fully opaque by default
    uint32_t client_version_ = 0;
    std::filesystem::path folder_path_;
    
    // ItemType storage and lookup
    std::vector<Domain::ItemType> items_;
    std::unordered_map<uint16_t, size_t> server_id_index_;
    
    // Sprite reader for secondary client
    std::unique_ptr<IO::SprReader> spr_reader_;
};

} // namespace MapEditor::Services

