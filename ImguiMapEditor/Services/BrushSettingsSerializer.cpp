#include "BrushSettingsSerializer.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <spdlog/spdlog.h>

namespace MapEditor::Services {

bool BrushSettingsSerializer::saveCustomBrushes(
    const std::string &filepath,
    const std::vector<CustomBrushShape> &customBrushes) {
  try {
    nlohmann::json j;
    j["version"] = 1;

    nlohmann::json brushesArray = nlohmann::json::array();
    for (const auto &brush : customBrushes) {
      nlohmann::json brushObj;
      brushObj["name"] = brush.name;
      brushObj["gridSize"] = brush.gridSize;

      // Flatten grid to 1D array for compact storage
      nlohmann::json gridData = nlohmann::json::array();
      for (const auto &row : brush.grid) {
        for (bool cell : row) {
          gridData.push_back(cell ? 1 : 0);
        }
      }
      brushObj["grid"] = gridData;

      brushesArray.push_back(brushObj);
    }
    j["brushes"] = brushesArray;

    std::ofstream file(filepath);
    if (!file.is_open()) {
      spdlog::error("Failed to open file for writing: {}", filepath);
      return false;
    }

    file << j.dump(2);
    spdlog::info("Saved {} custom brushes to {}", customBrushes.size(),
                 filepath);
    return true;

  } catch (const std::exception &e) {
    spdlog::error("Failed to save custom brushes: {}", e.what());
    return false;
  }
}

bool BrushSettingsSerializer::loadCustomBrushes(
    const std::string &filepath,
    std::vector<CustomBrushShape> &customBrushes) {
  try {
    std::ifstream file(filepath);
    if (!file.is_open()) {
      // File doesn't exist yet - that's okay
      spdlog::debug("Custom brushes file not found: {}", filepath);
      return true;
    }

    nlohmann::json j;
    file >> j;

    int version = j.value("version", 1);
    if (version != 1) {
      spdlog::warn("Unknown custom brushes file version: {}", version);
    }

    customBrushes.clear();

    for (const auto &brushObj : j["brushes"]) {
      CustomBrushShape brush;
      brush.name = brushObj["name"].get<std::string>();
      brush.gridSize = brushObj.value("gridSize", BrushSettingsService::DEFAULT_CUSTOM_GRID_SIZE);

      // Reconstruct grid from 1D array
      brush.grid.resize(brush.gridSize,
                        std::vector<bool>(brush.gridSize, false));

      const auto &gridData = brushObj["grid"];
      size_t idx = 0;
      for (int y = 0; y < brush.gridSize && idx < gridData.size(); ++y) {
        for (int x = 0; x < brush.gridSize && idx < gridData.size();
             ++x, ++idx) {
          brush.grid[y][x] = gridData[idx].get<int>() != 0;
        }
      }

      brush.computeOffsets();
      customBrushes.push_back(std::move(brush));
    }

    spdlog::info("Loaded {} custom brushes from {}", customBrushes.size(),
                 filepath);
    return true;

  } catch (const std::exception &e) {
    spdlog::error("Failed to load custom brushes: {}", e.what());
    return false;
  }
}

} // namespace MapEditor::Services
