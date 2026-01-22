#pragma once
#include "Domain/ItemType.h"
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

namespace MapEditor {
namespace IO {

/**
 * OTB (Open Tibia Binary) file version info
 */
struct OtbVersionInfo {
    uint32_t major_version = 0;
    uint32_t minor_version = 0;  // Corresponds to client version
    uint32_t build_number = 0;
    bool valid = false;
};

/**
 * Result of OTB parsing
 */
struct OtbResult {
    bool success = false;
    OtbVersionInfo version;
    std::vector<Domain::ItemType> items;
    std::string error;
};

/**
 * Reads items.otb files
 * Loads server-side item definitions with server ID -> client ID mapping
 */
class OtbReader {
public:
    /**
     * Read an items.otb file
     * @param path Path to items.otb
     * @return Result with items and version info
     */
    static OtbResult read(const std::filesystem::path& path);
    
    /**
     * Read only the version info (faster than full read)
     * @param path Path to items.otb
     * @return Version info
     */
    static OtbVersionInfo readVersionInfo(const std::filesystem::path& path);

private:
    // OTB attribute types
    enum class OtbAttribute : uint8_t {
        ServerId = 0x10,
        ClientId = 0x11,
        Name = 0x12,
        Description = 0x13,
        Speed = 0x14,
        SpriteHash = 0x20,
        MinimapColor = 0x21,
        MaxReadWriteChars = 0x22,
        MaxReadChars = 0x23,
        Light = 0x2A,
        StackOrder = 0x2B,
        TradeAs = 0x2D
    };
    
    // Root attribute for version
    enum class RootAttribute : uint8_t {
        Version = 0x01
    };
};

} // namespace IO
} // namespace MapEditor
