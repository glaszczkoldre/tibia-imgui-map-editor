#pragma once
#include "../BinaryReader.h"
#include "IO/Loader/SourceFragments.h"
#include <memory>
#include <vector>
#include <unordered_map>

namespace MapEditor {
namespace IO {

/**
 * Result of DAT parsing
 */
struct DatResult {
    bool success = false;
    uint32_t signature = 0;
    uint16_t max_item_id = 0;
    uint16_t max_outfit_id = 0;
    uint16_t max_effect_id = 0;
    uint16_t max_missile_id = 0;
    
    std::vector<ClientItem> items;
    std::vector<ClientItem> outfits;
    std::vector<ClientItem> effects;
    std::vector<ClientItem> missiles;
    
    std::string error;
};

/**
 * Abstract base class for version-specific DAT readers
 */
class DatReaderBase {
public:
    virtual ~DatReaderBase() = default;
    
    /**
     * Read DAT file
     */
    DatResult read(const std::filesystem::path& path,
                   uint32_t expected_signature = 0);
    
    /**
     * Get version range this reader supports
     */
    virtual std::pair<uint32_t, uint32_t> getVersionRange() const = 0;
    
    /**
     * Get reader name for debugging
     */
    virtual const char* getName() const = 0;

    /**
     * Whether this version uses extended (32-bit) sprite IDs
     */
    virtual bool usesExtendedSprites() const { return false; }
    
    /**
     * Whether this version has frame duration data
     */
    virtual bool hasFrameDurations() const { return false; }
    
    /**
     * Whether this version uses frame groups (10.50+ for outfits)
     */
    virtual bool hasFrameGroups() const { return false; }

protected:
    /**
     * Version-specific flag reading.
     * Default implementation uses a loop that calls transformFlag, handles standard flags,
     * and calls handleSpecificFlag for custom ones.
     */
    virtual void readItemFlags(ClientItem& item, BinaryReader& reader);

    /**
     * Transforms a raw flag to a canonical flag.
     * Override this to map version-specific flags to CanonicalFlags.
     */
    virtual uint8_t transformFlag(uint8_t raw) { return raw; }

    /**
     * Handles flags that are not covered by the standard switch case or require
     * version-specific processing (like HAS_OFFSET in older versions).
     * Returns true if the flag was handled, false otherwise.
     */
    virtual bool handleSpecificFlag(uint8_t flag, ClientItem& item, BinaryReader& reader) { return false; }
    
    /**
     * Whether this version reads patternZ from file (false for 7.10-7.54)
     */
    virtual bool shouldReadPatternZ() const { return true; }

private:
    void readCategory(BinaryReader& reader, std::vector<ClientItem>& items,
                      DatCategory category, uint16_t min_id, uint16_t max_id);
    void readSpriteData(ClientItem& item, BinaryReader& reader);
};

} // namespace IO
} // namespace MapEditor
