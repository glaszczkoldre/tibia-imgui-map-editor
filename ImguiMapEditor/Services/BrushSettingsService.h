#pragma once

#include "Domain/Position.h"
#include <array>
#include <functional>
#include <span>
#include <utility>
#include <vector>

namespace MapEditor::Services {

class ConfigService;

enum class BrushShape {
  Square,
  Circle,
};

struct BrushFootprint {
  BrushShape shape = BrushShape::Square;
  int size_x = 1;
  int size_y = 1;
  bool exact = true;
  bool aspect_locked = true;
  int span_x = 1;
  int span_y = 1;
  int min_offset_x = 0;
  int max_offset_x = 0;
  int min_offset_y = 0;
  int max_offset_y = 0;

  [[nodiscard]] bool containsOffset(int dx, int dy) const;
  [[nodiscard]] int legacySize() const;
};

class BrushSettingsService {
public:
  static constexpr int MIN_RADIUS_AXIS_SIZE = 0;
  static constexpr int MIN_EXACT_AXIS_SIZE = 1;
  static constexpr int MAX_AXIS_SIZE = 15;
  static constexpr int MIN_SPAWN_RADIUS = 1;
  static constexpr int MAX_SPAWN_RADIUS = 15;
  static constexpr std::array<int, 7> LEGACY_SIZE_PROGRESSION{0, 1, 2, 4, 6,
                                                              8, 11};

  using OnSettingsChangedCallback = std::function<void()>;

  BrushSettingsService() = default;
  ~BrushSettingsService() = default;
  BrushSettingsService(const BrushSettingsService &) = delete;
  BrushSettingsService &operator=(const BrushSettingsService &) = delete;

  void setBrushShape(BrushShape shape);
  [[nodiscard]] BrushShape getBrushShape() const { return shape_; }

  void setBrushSizeX(int size);
  void setBrushSizeY(int size);
  void setBrushSizeAxes(int sizeX, int sizeY);
  [[nodiscard]] int getBrushSizeX() const { return size_x_; }
  [[nodiscard]] int getBrushSizeY() const { return size_y_; }

  void setExactBrushSize(bool exact);
  [[nodiscard]] bool isExactBrushSize() const { return exact_; }

  void setBrushAspectRatioLocked(bool locked);
  [[nodiscard]] bool isBrushAspectRatioLocked() const {
    return aspect_locked_;
  }

  void setStandardSize(int size);
  void adjustSize(int delta);
  [[nodiscard]] int getStandardSize() const;
  void increaseSize();
  void decreaseSize();
  [[nodiscard]] static std::span<const int> getStandardSizeProgression() {
    return LEGACY_SIZE_PROGRESSION;
  }

  [[nodiscard]] int getEffectiveWidth() const;
  [[nodiscard]] int getEffectiveHeight() const;
  [[nodiscard]] int getEffectiveAxisSpanX() const;
  [[nodiscard]] int getEffectiveAxisSpanY() const;
  [[nodiscard]] BrushFootprint getBrushFootprint(bool forceSquare = false) const;
  [[nodiscard]] BrushFootprint getSquareBrushFootprint() const;
  [[nodiscard]] std::vector<std::pair<int, int>> getBrushOffsets(bool forceSquare = false) const;
  [[nodiscard]] std::vector<std::pair<int, int>> getSquareBrushOffsets() const;
  [[nodiscard]] std::vector<Domain::Position>
  getBrushPositions(const Domain::Position &center, bool forceSquare = false) const;

  void setPreviewBorder(bool enabled);
  [[nodiscard]] bool getPreviewBorder() const { return preview_border_; }

  void setAutoBorder(bool enabled);
  [[nodiscard]] bool getAutoBorder() const { return auto_border_; }
  void toggleAutoBorder();

  void setLockDoors(bool enabled);
  [[nodiscard]] bool getLockDoors() const { return lock_doors_; }

  void setEraserLeaveUniqueItems(bool enabled);
  [[nodiscard]] bool getEraserLeaveUniqueItems() const {
    return eraser_leave_unique_items_;
  }

  void setRawLikeSimone(bool enabled);
  [[nodiscard]] bool getRawLikeSimone() const { return raw_like_simone_; }

  void setAutoCreateSpawn(bool enabled);
  [[nodiscard]] bool getAutoCreateSpawn() const { return auto_create_spawn_; }

  void setDefaultSpawnRadius(int radius);
  [[nodiscard]] int getDefaultSpawnRadius() const {
    return default_spawn_radius_;
  }

  void setDefaultSpawnTime(int seconds);
  [[nodiscard]] int getDefaultSpawnTime() const { return default_spawn_time_; }

  void loadFromConfig(const ConfigService &config);
  void saveToConfig(ConfigService &config) const;

  void setOnSettingsChanged(OnSettingsChangedCallback callback) {
    on_settings_changed_ = std::move(callback);
  }

private:
  BrushShape shape_ = BrushShape::Square;
  int size_x_ = 1;
  int size_y_ = 1;
  bool exact_ = true;
  bool aspect_locked_ = true;

  bool preview_border_ = true;
  bool auto_border_ = true;
  bool lock_doors_ = false;
  bool eraser_leave_unique_items_ = true;
  bool raw_like_simone_ = true;
  bool auto_create_spawn_ = false;
  int default_spawn_radius_ = 3;
  int default_spawn_time_ = 60;

  OnSettingsChangedCallback on_settings_changed_;

  [[nodiscard]] int normalizeAxisValue(int value) const;
  [[nodiscard]] int nextLegacySize() const;
  [[nodiscard]] int previousLegacySize() const;
  void notifyChanged();
};

} // namespace MapEditor::Services
