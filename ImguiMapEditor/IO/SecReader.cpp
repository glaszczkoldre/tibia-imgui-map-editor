#include "SecReader.h"
#include "Sec/SecTileParser.h"
#include "ScriptReader.h"
#include "Services/ClientDataService.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <vector>
#include <cstdio>

namespace MapEditor {
namespace IO {

namespace {

struct SectorFile {
    std::filesystem::path path;
    int x, y, z;
};

} // anonymous namespace

SecResult SecReader::read(
    const std::filesystem::path& directory,
    Domain::ChunkedMap& map,
    Services::ClientDataService* client_data,
    SecProgressCallback progress) {
    
    SecResult result;
    
    if (!client_data) {
        result.error = "ClientDataService is required for SEC loading";
        return result;
    }
    
    if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
        result.error = "Directory does not exist: " + directory.string();
        return result;
    }
    
    if (progress) progress(0, "Scanning for sector files...");
    
    // Collect all .sec files
    std::vector<SectorFile> sector_files;
    
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) continue;
        
        auto ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), 
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        
        if (ext != ".sec") continue;
        
        int x, y, z;
        if (parseFilename(entry.path().filename().string(), x, y, z)) {
            sector_files.push_back({entry.path(), x, y, z});
            
            // Update bounds
            result.sector_x_min = std::min(result.sector_x_min, x);
            result.sector_x_max = std::max(result.sector_x_max, x);
            result.sector_y_min = std::min(result.sector_y_min, y);
            result.sector_y_max = std::max(result.sector_y_max, y);
            result.sector_z_min = std::min(result.sector_z_min, z);
            result.sector_z_max = std::max(result.sector_z_max, z);
        }
    }
    
    if (sector_files.empty()) {
        result.error = "No .sec files found in directory";
        return result;
    }
    
    result.sector_count = sector_files.size();
    spdlog::info("SecReader: Found {} sector files", sector_files.size());
    
    // Sort by Z, Y, X for consistent loading order
    std::sort(sector_files.begin(), sector_files.end(), 
        [](const SectorFile& a, const SectorFile& b) {
            if (a.z != b.z) return a.z < b.z;
            if (a.y != b.y) return a.y < b.y;
            return a.x < b.x;
        });
    
    // Calculate map size based on sector bounds
    int width = (result.sector_x_max - result.sector_x_min + 1) * 32;
    int height = (result.sector_y_max - result.sector_y_min + 1) * 32;
    map.setSize(static_cast<uint16_t>(std::min(width, 65535)), 
                static_cast<uint16_t>(std::min(height, 65535)));
    
    if (progress) progress(5, "Loading sectors...");
    
    // Load each sector
    size_t loaded = 0;
    for (const auto& sf : sector_files) {
        if (!readSector(sf.path, sf.x, sf.y, sf.z, map, client_data, result)) {
            spdlog::warn("SecReader: Failed to load sector {}-{}-{}", sf.x, sf.y, sf.z);
        }
        
        ++loaded;
        if (progress && loaded % 10 == 0) {
            int percent = 5;
            if (!sector_files.empty()) {
                percent += static_cast<int>(90.0 * loaded / sector_files.size());
            }
            progress(std::min(95, percent), "Loading sectors...");
        }
    }
    
    if (progress) progress(100, "SEC map loading complete");
    
    result.success = true;
    spdlog::info("SecReader: Loaded {} sectors, {} tiles, {} items",
                 result.sector_count, result.tile_count, result.item_count);
    
    return result;
}

SecResult SecReader::scanBounds(const std::filesystem::path& directory) {
    SecResult result;
    
    if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
        result.error = "Directory does not exist";
        return result;
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) continue;
        
        auto ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        
        if (ext != ".sec") continue;
        
        int x, y, z;
        if (parseFilename(entry.path().filename().string(), x, y, z)) {
            result.sector_count++;
            result.sector_x_min = std::min(result.sector_x_min, x);
            result.sector_x_max = std::max(result.sector_x_max, x);
            result.sector_y_min = std::min(result.sector_y_min, y);
            result.sector_y_max = std::max(result.sector_y_max, y);
            result.sector_z_min = std::min(result.sector_z_min, z);
            result.sector_z_max = std::max(result.sector_z_max, z);
        }
    }
    
    if (result.sector_count > 0) {
        result.success = true;
    } else {
        result.error = "No .sec files found";
    }
    
    return result;
}

bool SecReader::readSector(
    const std::filesystem::path& file,
    int sector_x, int sector_y, int sector_z,
    Domain::ChunkedMap& map,
    Services::ClientDataService* client_data,
    SecResult& result) {
    
    ScriptReader script;
    if (!script.open(file.string())) {
        spdlog::warn("SecReader: Failed to open {}", file.string());
        return false;
    }
    
    spdlog::debug("SecReader: Loading sector {}-{}-{}", sector_x, sector_y, sector_z);
    
    bool success = SecTileParser::parseSector(script, sector_x, sector_y, sector_z,
                                               map, client_data, result);
    
    script.close();
    return success;
}

bool SecReader::parseFilename(const std::string& filename, int& out_x, int& out_y, int& out_z) {
    // Expected format: XXXX-YYYY-ZZ.sec (variable width numbers)
    // Examples: 1015-0996-03.sec, 32-44-7.sec
    
    // Check extension
    if (filename.size() < 5) return false;
    std::string ext = filename.substr(filename.size() - 4);
    if (ext != ".sec" && ext != ".SEC") return false;
    
    std::string base = filename.substr(0, filename.length() - 4);
    
    // Find separators from the end (z is last, then y, then x)
    size_t z_pos = base.rfind('-');
    if (z_pos == std::string::npos || z_pos == 0) return false;
    
    size_t y_pos = base.rfind('-', z_pos - 1);
    if (y_pos == std::string::npos) return false;
    
    std::string x_str = base.substr(0, y_pos);
    std::string y_str = base.substr(y_pos + 1, z_pos - y_pos - 1);
    std::string z_str = base.substr(z_pos + 1);
    
    try {
        out_x = std::stoi(x_str);
        out_y = std::stoi(y_str);
        out_z = std::stoi(z_str);
    } catch (const std::invalid_argument&) {
        return false;
    } catch (const std::out_of_range&) {
        return false;
    }
    return true;
}

} // namespace IO
} // namespace MapEditor
