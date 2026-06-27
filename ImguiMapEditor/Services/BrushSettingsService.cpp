#include "BrushSettingsService.h"

#include "Services/ConfigService.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace MapEditor::Services {

namespace {

constexpr const char *kBrushShapeKey = "brush.shape";
constexpr const char *kBrushSizeXKey = "brush.size_x";
constexpr const char *kBrushSizeYKey = "brush.size_y";
constexpr const char *kBrushExactKey = "brush.exact_size";
constexpr const char *kBrushAspectLockedKey = "brush.aspect_locked";
constexpr const char *kPreviewBorderKey = "brush.preview_border";
constexpr const char *kAutoBorderKey = "brush.autoborder";
constexpr const char *kLockDoorsKey = "brush.lock_doors";
constexpr const char *kEraserLeaveUniqueItemsKey =
    "brush.eraser_leave_unique_items";
constexpr const char *kRawLikeSimoneKey = "brush.raw_like_simone";
constexpr const char *kAutoCreateSpawnKey = "brush.auto_create_spawn";
constexpr const char *kDefaultSpawnRadiusKey = "brush.default_spawn_radius";
constexpr const char *kDefaultSpawnTimeKey = "brush.default_spawn_time";
constexpr double kEllipseEpsilon = 0.005;

[[nodiscard]] int computeSpan(int value, bool exact) {
  return exact ? std::max(1, value) : std::max(0, value) * 2 + 1;
}

[[nodiscard]] int normalizeAxisValue(int value, bool exact) {
  const int minValue = exact ? BrushSettingsService::MIN_EXACT_AXIS_SIZE
                             : BrushSettingsService::MIN_RADIUS_AXIS_SIZE;
  return std::clamp(value, minValue, BrushSettingsService::MAX_AXIS_SIZE);
}

[[nodiscard]] int nextSizeFrom(int current) {
  for (const auto candidate : BrushSettingsService::LEGACY_SIZE_PROGRESSION) {
    if (candidate > current) {
      return candidate;
    }
  }
  return BrushSettingsService::LEGACY_SIZE_PROGRESSION.back();
}

[[nodiscard]] int previousSizeFrom(int current) {
  for (auto it = BrushSettingsService::LEGACY_SIZE_PROGRESSION.rbegin();
       it != BrushSettingsService::LEGACY_SIZE_PROGRESSION.rend(); ++it) {
    if (*it < current) {
      return *it;
    }
  }
  return BrushSettingsService::LEGACY_SIZE_PROGRESSION.front();
}

[[nodiscard]] double safeNormalize(double delta, double radius) {
  if (radius <= 0.0) {
    return delta == 0.0 ? 0.0 : std::numeric_limits<double>::infinity();
  }
  return delta / radius;
}

} // namespace

bool BrushFootprint::containsOffset(int dx, int dy) const {
  if (dx < min_offset_x || dx > max_offset_x || dy < min_offset_y ||
      dy > max_offset_y) {
    return false;
  }

  if (shape == BrushShape::Square) {
    return true;
  }

  if (!exact) {
    const double nx =
        safeNormalize(static_cast<double>(dx), static_cast<double>(size_x));
    const double ny =
        safeNormalize(static_cast<double>(dy), static_cast<double>(size_y));
    return nx * nx + ny * ny < 1.0 + kEllipseEpsilon;
  }

  const double centerX =
      (static_cast<double>(min_offset_x) + static_cast<double>(max_offset_x)) /
      2.0;
  const double centerY =
      (static_cast<double>(min_offset_y) + static_cast<double>(max_offset_y)) /
      2.0;
  const double radiusX = static_cast<double>(span_x) / 2.0;
  const double radiusY = static_cast<double>(span_y) / 2.0;
  const double nx = safeNormalize(static_cast<double>(dx) - centerX, radiusX);
  const double ny = safeNormalize(static_cast<double>(dy) - centerY, radiusY);
  return nx * nx + ny * ny <= 1.0 + kEllipseEpsilon;
}

int BrushFootprint::legacySize() const {
  if (exact && size_x == 1 && size_y == 1) {
    return 0;
  }
  return std::max(size_x, size_y);
}

void BrushSettingsService::setBrushShape(BrushShape shape) {
  if (shape_ == shape) {
    return;
  }
  shape_ = shape;
  notifyChanged();
}

void BrushSettingsService::setBrushSizeX(int size) {
  const int normalized = normalizeAxisValue(size);
  setBrushSizeAxes(normalized, aspect_locked_ ? normalized : size_y_);
}

void BrushSettingsService::setBrushSizeY(int size) {
  const int normalized = normalizeAxisValue(size);
  setBrushSizeAxes(aspect_locked_ ? normalized : size_x_, normalized);
}

void BrushSettingsService::setBrushSizeAxes(int sizeX, int sizeY) {
  const int normalizedX = normalizeAxisValue(sizeX);
  const int normalizedY = normalizeAxisValue(sizeY);
  if (size_x_ == normalizedX && size_y_ == normalizedY) {
    return;
  }

  size_x_ = normalizedX;
  size_y_ = normalizedY;
  notifyChanged();
}

void BrushSettingsService::setExactBrushSize(bool exact) {
  if (exact_ == exact) {
    return;
  }

  exact_ = exact;
  size_x_ = normalizeAxisValue(size_x_);
  size_y_ = normalizeAxisValue(size_y_);
  notifyChanged();
}

void BrushSettingsService::setBrushAspectRatioLocked(bool locked) {
  if (aspect_locked_ == locked) {
    return;
  }

  aspect_locked_ = locked;
  if (aspect_locked_ && size_x_ != size_y_) {
    size_y_ = size_x_;
  }
  notifyChanged();
}

void BrushSettingsService::setStandardSize(int size) {
  const int normalized = MapEditor::Services::normalizeAxisValue(size, false);
  const bool changed =
      size_x_ != normalized || size_y_ != normalized || exact_ ||
      !aspect_locked_;

  size_x_ = normalized;
  size_y_ = normalized;
  exact_ = false;
  aspect_locked_ = true;

  if (changed) {
    notifyChanged();
  }
}

void BrushSettingsService::adjustSize(int delta) {
  if (delta == 0) {
    return;
  }

  const int minValue = exact_ ? MIN_EXACT_AXIS_SIZE : MIN_RADIUS_AXIS_SIZE;
  const int maxValue = MAX_AXIS_SIZE;

  int actualDelta = delta;
  if (delta < 0) {
    int limitDecrease = std::min(size_x_ - minValue, size_y_ - minValue);
    actualDelta = -std::min(-delta, limitDecrease);
  } else {
    int limitIncrease = std::min(maxValue - size_x_, maxValue - size_y_);
    actualDelta = std::min(delta, limitIncrease);
  }

  if (actualDelta != 0) {
    size_x_ += actualDelta;
    size_y_ += actualDelta;
    notifyChanged();
  }
}

int BrushSettingsService::getStandardSize() const {
  return getBrushFootprint().legacySize();
}

void BrushSettingsService::increaseSize() {
  setStandardSize(nextLegacySize());
}

void BrushSettingsService::decreaseSize() {
  setStandardSize(previousLegacySize());
}

int BrushSettingsService::getEffectiveWidth() const {
  return getEffectiveAxisSpanX();
}

int BrushSettingsService::getEffectiveHeight() const {
  return getEffectiveAxisSpanY();
}

int BrushSettingsService::getEffectiveAxisSpanX() const {
  return computeSpan(size_x_, exact_);
}

int BrushSettingsService::getEffectiveAxisSpanY() const {
  return computeSpan(size_y_, exact_);
}

BrushFootprint BrushSettingsService::getBrushFootprint(bool forceSquare) const {
  BrushFootprint footprint;
  footprint.shape = forceSquare ? BrushShape::Square : shape_;
  footprint.size_x = normalizeAxisValue(size_x_);
  footprint.size_y = normalizeAxisValue(size_y_);
  footprint.exact = exact_;
  footprint.aspect_locked = aspect_locked_;
  footprint.span_x = computeSpan(footprint.size_x, exact_);
  footprint.span_y = computeSpan(footprint.size_y, exact_);

  if (!exact_) {
    footprint.min_offset_x = -footprint.size_x;
    footprint.max_offset_x = footprint.size_x;
    footprint.min_offset_y = -footprint.size_y;
    footprint.max_offset_y = footprint.size_y;
    return footprint;
  }

  const auto assignAxis = [](int span, int &minOffset, int &maxOffset) {
    if (span % 2 == 1) {
      minOffset = -(span / 2);
      maxOffset = span / 2;
      return;
    }

    minOffset = -(span - 1);
    maxOffset = 0;
  };

  assignAxis(footprint.span_x, footprint.min_offset_x,
             footprint.max_offset_x);
  assignAxis(footprint.span_y, footprint.min_offset_y,
             footprint.max_offset_y);
  return footprint;
}

BrushFootprint BrushSettingsService::getSquareBrushFootprint() const {
  return getBrushFootprint(true);
}

std::vector<std::pair<int, int>> BrushSettingsService::getBrushOffsets(bool forceSquare) const {
  const auto footprint = getBrushFootprint(forceSquare);
  std::vector<std::pair<int, int>> offsets;
  offsets.reserve(static_cast<size_t>(footprint.span_x * footprint.span_y));

  for (int dy = footprint.min_offset_y; dy <= footprint.max_offset_y; ++dy) {
    for (int dx = footprint.min_offset_x; dx <= footprint.max_offset_x; ++dx) {
      if (footprint.containsOffset(dx, dy)) {
        offsets.emplace_back(dx, dy);
      }
    }
  }

  if (offsets.empty()) {
    offsets.emplace_back(0, 0);
  }
  return offsets;
}

std::vector<std::pair<int, int>>
BrushSettingsService::getSquareBrushOffsets() const {
  return getBrushOffsets(true);
}

std::vector<Domain::Position>
BrushSettingsService::getBrushPositions(const Domain::Position &center, bool forceSquare) const {
  std::vector<Domain::Position> positions;
  const auto offsets = getBrushOffsets(forceSquare);
  positions.reserve(offsets.size());
  for (const auto &[dx, dy] : offsets) {
    positions.emplace_back(center.x + dx, center.y + dy, center.z);
  }
  return positions;
}

void BrushSettingsService::setPreviewBorder(bool enabled) {
  if (preview_border_ == enabled) {
    return;
  }
  preview_border_ = enabled;
  notifyChanged();
}

void BrushSettingsService::setAutoBorder(bool enabled) {
  if (auto_border_ == enabled) {
    return;
  }
  auto_border_ = enabled;
  notifyChanged();
}

void BrushSettingsService::toggleAutoBorder() {
  setAutoBorder(!auto_border_);
}

void BrushSettingsService::setLockDoors(bool enabled) {
  if (lock_doors_ == enabled) {
    return;
  }
  lock_doors_ = enabled;
  notifyChanged();
}

void BrushSettingsService::setEraserLeaveUniqueItems(bool enabled) {
  if (eraser_leave_unique_items_ == enabled) {
    return;
  }
  eraser_leave_unique_items_ = enabled;
  notifyChanged();
}

void BrushSettingsService::setRawLikeSimone(bool enabled) {
  if (raw_like_simone_ == enabled) {
    return;
  }
  raw_like_simone_ = enabled;
  notifyChanged();
}

void BrushSettingsService::setAutoCreateSpawn(bool enabled) {
  if (auto_create_spawn_ == enabled) {
    return;
  }
  auto_create_spawn_ = enabled;
  notifyChanged();
}

void BrushSettingsService::setDefaultSpawnRadius(int radius) {
  radius = std::clamp(radius, MIN_SPAWN_RADIUS, MAX_SPAWN_RADIUS);
  if (default_spawn_radius_ == radius) {
    return;
  }
  default_spawn_radius_ = radius;
  notifyChanged();
}

void BrushSettingsService::setDefaultSpawnTime(int seconds) {
  seconds = std::clamp(seconds, 0, 86400);
  if (default_spawn_time_ == seconds) {
    return;
  }
  default_spawn_time_ = seconds;
  notifyChanged();
}

void BrushSettingsService::loadFromConfig(const ConfigService &config) {
  const auto shapeName = config.get<std::string>(kBrushShapeKey, "square");
  shape_ = shapeName == "circle" ? BrushShape::Circle : BrushShape::Square;
  exact_ = config.get<bool>(kBrushExactKey, true);
  aspect_locked_ = config.get<bool>(kBrushAspectLockedKey, true);
  size_x_ = normalizeAxisValue(config.get<int>(kBrushSizeXKey, 1));
  size_y_ = aspect_locked_
                ? size_x_
                : normalizeAxisValue(config.get<int>(kBrushSizeYKey, 1));
  preview_border_ = config.get<bool>(kPreviewBorderKey, true);
  auto_border_ = config.get<bool>(kAutoBorderKey, true);
  lock_doors_ = config.get<bool>(kLockDoorsKey, false);
  eraser_leave_unique_items_ =
      config.get<bool>(kEraserLeaveUniqueItemsKey, true);
  raw_like_simone_ = config.get<bool>(kRawLikeSimoneKey, true);
  auto_create_spawn_ = config.get<bool>(kAutoCreateSpawnKey, false);
  default_spawn_radius_ =
      std::clamp(config.get<int>(kDefaultSpawnRadiusKey, 3),
                 MIN_SPAWN_RADIUS, MAX_SPAWN_RADIUS);
  default_spawn_time_ =
      std::clamp(config.get<int>(kDefaultSpawnTimeKey, 60), 0, 86400);
  notifyChanged();
}

void BrushSettingsService::saveToConfig(ConfigService &config) const {
  config.set(kBrushShapeKey, shape_ == BrushShape::Circle ? "circle" : "square");
  config.set(kBrushSizeXKey, size_x_);
  config.set(kBrushSizeYKey, size_y_);
  config.set(kBrushExactKey, exact_);
  config.set(kBrushAspectLockedKey, aspect_locked_);
  config.set(kPreviewBorderKey, preview_border_);
  config.set(kAutoBorderKey, auto_border_);
  config.set(kLockDoorsKey, lock_doors_);
  config.set(kEraserLeaveUniqueItemsKey, eraser_leave_unique_items_);
  config.set(kRawLikeSimoneKey, raw_like_simone_);
  config.set(kAutoCreateSpawnKey, auto_create_spawn_);
  config.set(kDefaultSpawnRadiusKey, default_spawn_radius_);
  config.set(kDefaultSpawnTimeKey, default_spawn_time_);
}

int BrushSettingsService::normalizeAxisValue(int value) const {
  return MapEditor::Services::normalizeAxisValue(value, exact_);
}

int BrushSettingsService::nextLegacySize() const {
  return nextSizeFrom(getStandardSize());
}

int BrushSettingsService::previousLegacySize() const {
  return previousSizeFrom(getStandardSize());
}

void BrushSettingsService::notifyChanged() {
  if (on_settings_changed_) {
    on_settings_changed_();
  }
}

} // namespace MapEditor::Services
