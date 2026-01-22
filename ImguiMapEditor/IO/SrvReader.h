#pragma once
#include "Domain/ItemType.h"
#include <filesystem>
#include <vector>
#include <string>

namespace MapEditor {
namespace IO {

/**
 * Result of SRV parsing
 */
struct SrvResult {
    bool success = false;
    std::vector<Domain::ItemType> items;
    std::string error;
};

/**
 * Reads items.srv files (ancient Tibia 7.0-7.7x format)
 * 
 * The SRV format is a text-based script format where:
 * - typeid = <id> defines an item
 * - name = "<name>" sets the item name
 * - flags = { flag1, flag2, ... } sets item properties
 * - attributes = { attr1=val, ... } sets item attributes
 * 
 * Note: In SRV format, server_id == client_id (they were the same in early Tibia)
 */
class SrvReader {
public:
    /**
     * Read an items.srv file
     * @param path Path to items.srv
     * @return Result with items
     */
    static SrvResult read(const std::filesystem::path& path);
};

} // namespace IO
} // namespace MapEditor
