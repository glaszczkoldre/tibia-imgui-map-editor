#include "BrushSettingsService.h"
#include "BrushSettingsSerializer.h"

#include "Services/ConfigService.h"
#include <cstdlib>
#include <fstream>
#include <ranges>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace MapEditor::Services {

namespace {

constexpr const char *kPreviewBorderKey = "brush.preview_border";
constexpr const char *kLockDoorsKey = "brush.lock_doors";
constexpr const char *kDoodadEraseMatchingOnlyKey =
    "brush.doodad_erase_matching_only";
constexpr const char *kEraserLeaveUniqueItemsKey =
    "brush.eraser_leave_unique_items";
constexpr const char *kAutoCreateSpawnKey = "brush.auto_create_spawn";
constexpr const char *kDefaultSpawnRadiusKey = "brush.default_spawn_radius";
constexpr const char *kDefaultSpawnTimeKey = "brush.default_spawn_time";
constexpr const char *kRawLikeSimoneKey = "brush.raw_like_simone";

[[nodiscard]] int snapToDiscreteBrushSize(int size) {
  size = std::clamp(size, BrushSettingsService::MIN_SIZE,
                    BrushSettingsService::MAX_SIZE);

  auto best = BrushSettingsService::STANDARD_SIZE_PROGRESSION.front();
  auto bestDistance = std::abs(best - size);
  for (const auto candidate :
       BrushSettingsService::STANDARD_SIZE_PROGRESSION) {
    const auto distance = std::abs(candidate - size);
    if (distance < bestDistance) {
      best = candidate;
      bestDistance = distance;
    }
  }
  return best;
}

[[nodiscard]] int nextDiscreteBrushSize(int current) {
  current = snapToDiscreteBrushSize(current);
  for (const auto candidate :
       BrushSettingsService::STANDARD_SIZE_PROGRESSION) {
    if (candidate > current) {
      return candidate;
    }
  }
  return BrushSettingsService::STANDARD_SIZE_PROGRESSION.back();
}

[[nodiscard]] int previousDiscreteBrushSize(int current) {
  current = snapToDiscreteBrushSize(current);
  for (auto it = BrushSettingsService::STANDARD_SIZE_PROGRESSION.rbegin();
       it != BrushSettingsService::STANDARD_SIZE_PROGRESSION.rend(); ++it) {
    if (*it < current) {
      return *it;
    }
  }
  return BrushSettingsService::STANDARD_SIZE_PROGRESSION.front();
}

} // namespace

int BrushSettingsService::normalizeStandardSize(int size) {
  return snapToDiscreteBrushSize(size);
}

int BrushSettingsService::getStandardSizeProgressionIndexForValue(int size) {
  const auto normalized = normalizeStandardSize(size);
  const auto it =
      std::ranges::find(STANDARD_SIZE_PROGRESSION, normalized);
  if (it == STANDARD_SIZE_PROGRESSION.end()) {
    return 0;
  }
  return static_cast<int>(std::distance(STANDARD_SIZE_PROGRESSION.begin(), it));
}

int BrushSettingsService::getNextStandardSize() const {
  return nextDiscreteBrushSize(standardSize_);
}

int BrushSettingsService::getPreviousStandardSize() const {
  return previousDiscreteBrushSize(standardSize_);
}

// ========================
// Brush Type
// ========================

void BrushSettingsService::setBrushType(BrushType type) {
  if (type_ != type) {
    type_ = type;
    notifyChanged();
  }
}

// ========================
// Size Mode
// ========================

void BrushSettingsService::setBrushSizeMode(BrushSizeMode mode) {
  if (sizeMode_ != mode) {
    sizeMode_ = mode;
    notifyChanged();
  }
}

void BrushSettingsService::setPreviewBorder(bool enabled) {
  if (previewBorder_ != enabled) {
    previewBorder_ = enabled;
    notifyChanged();
  }
}

void BrushSettingsService::setLockDoors(bool enabled) {
  if (lockDoors_ != enabled) {
    lockDoors_ = enabled;
    notifyChanged();
  }
}

// ========================
// Standard Size (now direct tile count, not radius)
// ========================

void BrushSettingsService::setStandardSize(int size) {
  size = std::clamp(size, MIN_SIZE, MAX_SIZE);

  if (std::ranges::find(STANDARD_SIZE_PROGRESSION, size) ==
      STANDARD_SIZE_PROGRESSION.end()) {
    if (size > standardSize_) {
      size = getNextStandardSize();
    } else if (size < standardSize_) {
      size = getPreviousStandardSize();
    } else {
      size = normalizeStandardSize(size);
    }
  }

  if (standardSize_ != size) {
    standardSize_ = size;
    notifyChanged();
  }
}

void BrushSettingsService::increaseSize() {
  setStandardSize(getNextStandardSize());
}

void BrushSettingsService::decreaseSize() {
  setStandardSize(getPreviousStandardSize());
}

// ========================
// Custom Dimensions
// ========================

void BrushSettingsService::setCustomDimensions(int width, int height) {
  width = std::clamp(width, MIN_SIZE, MAX_SIZE);
  height = std::clamp(height, MIN_SIZE, MAX_SIZE);

  if (customWidth_ != width || customHeight_ != height) {
    customWidth_ = width;
    customHeight_ = height;
    notifyChanged();
  }
}

// ========================
// Computed Properties
// ========================

int BrushSettingsService::getEffectiveWidth() const {
  if (type_ == BrushType::Custom) {
    const auto *brush = getSelectedCustomBrush();
    if (brush && !brush->offsets.empty()) {
      int minX = 0, maxX = 0;
      for (const auto &[dx, dy] : brush->offsets) {
        minX = std::min(minX, dx);
        maxX = std::max(maxX, dx);
      }
      return maxX - minX + 1;
    }
    return 1;
  }

  if (sizeMode_ == BrushSizeMode::CustomDimensions) {
    return customWidth_;
  }

  // Standard size: direct tile count (size=3 means 3 tiles wide)
  return standardSize_;
}

int BrushSettingsService::getEffectiveHeight() const {
  if (type_ == BrushType::Custom) {
    const auto *brush = getSelectedCustomBrush();
    if (brush && !brush->offsets.empty()) {
      int minY = 0, maxY = 0;
      for (const auto &[dx, dy] : brush->offsets) {
        minY = std::min(minY, dy);
        maxY = std::max(maxY, dy);
      }
      return maxY - minY + 1;
    }
    return 1;
  }

  if (sizeMode_ == BrushSizeMode::CustomDimensions) {
    return customHeight_;
  }

  // Standard size: direct tile count (size=3 means 3 tiles tall)
  return standardSize_;
}

// ========================
// Custom Brushes
// ========================

void BrushSettingsService::addCustomBrush(const CustomBrushShape &brush) {
  // Check if brush with same name exists
  for (auto &existing : customBrushes_) {
    if (existing.name == brush.name) {
      existing = brush;
      existing.computeOffsets();
      notifyChanged();
      return;
    }
  }

  // Add new brush
  customBrushes_.push_back(brush);
  customBrushes_.back().computeOffsets();
  notifyChanged();
}

void BrushSettingsService::removeCustomBrush(const std::string &name) {
  auto it = std::remove_if(
      customBrushes_.begin(), customBrushes_.end(),
      [&name](const CustomBrushShape &b) { return b.name == name; });

  if (it != customBrushes_.end()) {
    customBrushes_.erase(it, customBrushes_.end());

    // Clear selection if removed brush was selected
    if (selectedCustomBrushName_ == name) {
      selectedCustomBrushName_.clear();
    }
    notifyChanged();
  }
}

void BrushSettingsService::selectCustomBrush(const std::string &name) {
  if (selectedCustomBrushName_ != name) {
    selectedCustomBrushName_ = name;
    notifyChanged();
  }
}

const CustomBrushShape *BrushSettingsService::getSelectedCustomBrush() const {
  if (selectedCustomBrushName_.empty()) {
    return nullptr;
  }

  for (const auto &brush : customBrushes_) {
    if (brush.name == selectedCustomBrushName_) {
      return &brush;
    }
  }
  return nullptr;
}

// ========================
// Core API: Position Calculation
// ========================

std::vector<Domain::Position>
BrushSettingsService::getBrushPositions(const Domain::Position &center) const {

  std::vector<Domain::Position> positions;
  auto offsets = getBrushOffsets();

  positions.reserve(offsets.size());
  for (const auto &[dx, dy] : offsets) {
    positions.emplace_back(center.x + dx, center.y + dy, center.z);
  }

  return positions;
}

std::vector<std::pair<int, int>> BrushSettingsService::getBrushOffsets() const {
  switch (type_) {
  case BrushType::Square:
    return calculateSquareOffsets();
  case BrushType::Circle:
    return calculateCircleOffsets();
  case BrushType::Custom:
    return calculateCustomOffsets();
  }
}

// ========================
// Position Calculation Helpers
// ========================

std::vector<std::pair<int, int>>
BrushSettingsService::calculateSquareOffsets() const {
  std::vector<std::pair<int, int>> offsets;

  int width, height;
  if (sizeMode_ == BrushSizeMode::CustomDimensions) {
    width = customWidth_;
    height = customHeight_;
  } else {
    // Standard: direct tile count (size=3 means 3x3 grid)
    width = height = standardSize_;
  }

  // Calculate half extents (center is at 0,0)
  int halfW = width / 2;
  int halfH = height / 2;

  // For even sizes, bias toward negative
  int startX = -halfW;
  int endX = width - halfW - 1;
  int startY = -halfH;
  int endY = height - halfH - 1;

  for (int dy = startY; dy <= endY; ++dy) {
    for (int dx = startX; dx <= endX; ++dx) {
      offsets.emplace_back(dx, dy);
    }
  }

  return offsets;
}

std::vector<std::pair<int, int>>
BrushSettingsService::calculateCircleOffsets() const {
  std::vector<std::pair<int, int>> offsets;

  int width, height;
  if (sizeMode_ == BrushSizeMode::CustomDimensions) {
    width = customWidth_;
    height = customHeight_;
  } else {
    // Standard: direct tile count
    width = height = standardSize_;
  }

  if (width == 1 && height == 1) {
    // Size 1 = single tile
    offsets.emplace_back(0, 0);
    return offsets;
  }

  // Calculate half extents for ellipse
  int halfW = width / 2;
  int halfH = height / 2;

  float rX = static_cast<float>(width) / 2.0f;
  float rY = static_cast<float>(height) / 2.0f;

  int startX = -halfW;
  int endX = width - halfW - 1;
  int startY = -halfH;
  int endY = height - halfH - 1;

  for (int dy = startY; dy <= endY; ++dy) {
    for (int dx = startX; dx <= endX; ++dx) {
      // Check if point is inside ellipse (with 0.5 offset for center of tile)
      float nx = (static_cast<float>(dx) + 0.5f) / rX;
      float ny = (static_cast<float>(dy) + 0.5f) / rY;

      if (nx * nx + ny * ny <= 1.0f) {
        offsets.emplace_back(dx, dy);
      }
    }
  }

  // Ensure at least center tile
  if (offsets.empty()) {
    offsets.emplace_back(0, 0);
  }

  return offsets;
}

std::vector<std::pair<int, int>>
BrushSettingsService::calculateCustomOffsets() const {
  const auto *brush = getSelectedCustomBrush();
  if (!brush || brush->offsets.empty()) {
    // No custom brush selected, return single tile
    return {{0, 0}};
  }

  return brush->offsets;
}

// ========================
// Persistence
// ========================

bool BrushSettingsService::saveCustomBrushes(
    const std::string &filepath) const {
  return BrushSettingsSerializer::saveCustomBrushes(filepath, customBrushes_);
}

bool BrushSettingsService::loadCustomBrushes(const std::string &filepath) {
  return BrushSettingsSerializer::loadCustomBrushes(filepath, customBrushes_);
}

void BrushSettingsService::loadFromConfig(const ConfigService &config) {
  previewBorder_ = config.get<bool>(kPreviewBorderKey, true);
  lockDoors_ = config.get<bool>(kLockDoorsKey, false);
  doodadEraseMatchingOnly_ =
      config.get<bool>(kDoodadEraseMatchingOnlyKey, true);
  eraserLeaveUniqueItems_ =
      config.get<bool>(kEraserLeaveUniqueItemsKey, true);
  rawLikeSimone_ = config.get<bool>(kRawLikeSimoneKey, true);
  autoCreateSpawn_ = config.get<bool>(kAutoCreateSpawnKey, false);
  defaultSpawnRadius_ =
      std::clamp(config.get<int>(kDefaultSpawnRadiusKey, 3), 1, 10);
  defaultSpawnTime_ =
      std::clamp(config.get<int>(kDefaultSpawnTimeKey, 60), 1, 86400);
  notifyChanged();
}

void BrushSettingsService::saveToConfig(ConfigService &config) const {
  config.set(kPreviewBorderKey, previewBorder_);
  config.set(kLockDoorsKey, lockDoors_);
  config.set(kDoodadEraseMatchingOnlyKey, doodadEraseMatchingOnly_);
  config.set(kEraserLeaveUniqueItemsKey, eraserLeaveUniqueItems_);
  config.set(kRawLikeSimoneKey, rawLikeSimone_);
  config.set(kAutoCreateSpawnKey, autoCreateSpawn_);
  config.set(kDefaultSpawnRadiusKey, defaultSpawnRadius_);
  config.set(kDefaultSpawnTimeKey, defaultSpawnTime_);
}

// ========================
// Change Notification
// ========================

void BrushSettingsService::notifyChanged() {
  if (onSettingsChanged_) {
    onSettingsChanged_();
  }
}

} // namespace MapEditor::Services
