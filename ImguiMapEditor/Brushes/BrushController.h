#pragma once
#include "Brushes/Core/IBrush.h"
#include "Brushes/Types/EraserBrush.h"
#include "Brushes/Types/FlagBrush.h"
#include "Brushes/Types/DoorBrush.h"
#include "Brushes/Types/HouseExitBrush.h"
#include "Brushes/Types/HouseBrush.h"
#include "Brushes/Types/OptionalBorderBrush.h"
#include "Brushes/Types/SpawnBrush.h"
#include "Brushes/Types/WaypointBrush.h"
#include "Domain/ChunkedMap.h"
#include "Domain/History/HistoryManager.h"
#include "Domain/Position.h"
#include "Services/Autoborder/AutoborderEngine.h"
#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <unordered_set>
#include <vector>

namespace MapEditor::Brushes {
class BrushRegistry;
}

namespace MapEditor::Domain {
class Item;
}

namespace MapEditor::Services {
class ClientDataService;
class BrushSettingsService;
namespace Preview {
class PreviewService;
class BrushPreviewFactory;
} // namespace Preview
} // namespace MapEditor::Services

namespace MapEditor::Brushes {

enum class BrushPickMode : uint8_t {
  Smart,
  Raw,
  Ground,
  Doodad,
  Collection,
  Door,
  Wall,
  Carpet,
  Table,
  Creature,
  Spawn,
  House,
  HouseExit,
  Waypoint,
  OptionalBorder,
  ProtectionZone,
  NoPvp,
  NoLogout,
  PvpZone
};

struct ResolvedBrushSelection {
  IBrush *brush = nullptr;
  BrushPickMode mode = BrushPickMode::Smart;
  std::string displayName;
  int variation = 0;
  std::optional<uint16_t> rawItemId;
  std::optional<uint32_t> houseId;
  std::optional<uint32_t> houseExitHouseId;
  std::optional<std::string> waypointName;

  [[nodiscard]] bool isValid() const noexcept {
    return brush != nullptr || rawItemId.has_value();
  }
};

// Callback type for notifying when brush becomes active (to clear selection)
using OnBrushActivatedCallback = std::function<void()>;
using TilesMutatedCallback =
    std::function<void(const std::vector<Domain::Position> &)>;

/**
 * Controls brush selection and application.
 *
 * Supports all brush types via IBrush interface.
 * Uses BrushPreviewFactory to create preview providers.
 */
class BrushController {
public:
  BrushController() noexcept = default;
  ~BrushController();

  // Non-copyable
  BrushController(const BrushController &) = delete;
  BrushController &operator=(const BrushController &) = delete;

  /**
   * Initialize with required dependencies.
   * @param map The map to operate on
   * @param historyManager The history manager for undo/redo
   * @param clientData Client data service for item type lookup
   */
  void initialize(Domain::ChunkedMap *map,
                  Domain::History::HistoryManager *historyManager,
                  Services::ClientDataService *clientData);

  void setBrushRegistry(BrushRegistry *registry);

  /**
   * Set the current brush.
   * Uses BrushPreviewFactory to create appropriate preview provider.
   * @param brush The brush to activate (non-owning pointer)
   */
  void setBrush(IBrush *brush);

  /**
   * Set callback for when brush is activated (to clear selection).
   */
  void setOnBrushActivatedCallback(OnBrushActivatedCallback callback) {
    onBrushActivated_ = std::move(callback);
  }

  void setTilesMutatedCallback(TilesMutatedCallback callback) {
    onTilesMutated_ = std::move(callback);
  }

  /**
   * Clear the current brush selection.
   */
  void clearBrush();
  bool restoreLastBrush();
  bool toggleSelectionTool();
  bool canRotateItemAt(const Domain::Position &pos,
                       const Domain::Item *preferredItem = nullptr) const;
  bool rotateItemAt(const Domain::Position &pos,
                    const Domain::Item *preferredItem = nullptr);
  bool canSwitchDoorAt(const Domain::Position &pos,
                       const Domain::Item *preferredItem = nullptr) const;
  bool switchDoorAt(const Domain::Position &pos,
                    const Domain::Item *preferredItem = nullptr);
  bool usesPreciseMutationNotifications() const;
  std::optional<bool> getDoorOpenStateAt(const Domain::Position &pos,
                                         const Domain::Item *preferredItem = nullptr) const;

  /**
   * Set preview service for brush preview updates.
   */
  void setPreviewService(Services::Preview::PreviewService *previewService) {
    previewService_ = previewService;
  }

  /**
   * Set preview factory for creating preview providers.
   */
  void setPreviewFactory(Services::Preview::BrushPreviewFactory *factory) {
    previewFactory_ = factory;
  }

  /**
   * Rebuild the active preview provider after session-scoped dependencies
   * change, without reactivating the current brush.
   */
  void refreshPreviewProvider();

  /**
   * Check if a brush is currently selected.
   */
  bool hasBrush() const { return currentBrush_ != nullptr; }

  /**
   * Get the current brush (for queries).
   */
  const IBrush *getCurrentBrush() const { return currentBrush_; }

  /**
   * Resolve and activate a brush from already-painted tile content.
   * Returns false when no matching brush can be inferred.
   */
  bool selectBrushFromTile(const Domain::Tile &tile,
                           BrushPickMode mode = BrushPickMode::Smart,
                           const Domain::Item *preferredItem = nullptr);

  /**
   * Resolve the brush that would be selected from painted tile content without
   * mutating editor selection state.
   */
  std::optional<ResolvedBrushSelection>
  resolveBrushFromTile(const Domain::Tile &tile,
                       BrushPickMode mode = BrushPickMode::Smart,
                       const Domain::Item *preferredItem = nullptr) const;

  [[nodiscard]] bool
  canSelectBrushFromTile(const Domain::Tile &tile,
                         BrushPickMode mode = BrushPickMode::Smart,
                         const Domain::Item *preferredItem = nullptr) const {
    return resolveBrushFromTile(tile, mode, preferredItem).has_value();
  }

  /**
   * Get the current brush item ID (for RAW brush compatibility).
   * Returns nullopt if not in RAW mode.
   */
  std::optional<uint32_t> getCurrentItemId() const;

  /**
   * Apply the current brush at a position.
   * Uses IBrush::draw() internally.
   * @param pos Position to apply brush
   * @return true if brush was applied
   */
  bool applyBrush(const Domain::Position &pos);
  bool applyBrush(const Domain::Position &pos, uint32_t modifiers);

  /**
   * Erase at a position using current brush.
   * Uses IBrush::undraw() internally.
   * @param pos Position to erase
   * @return true if erase was performed
   */
  bool eraseBrush(const Domain::Position &pos);
  bool eraseBrush(const Domain::Position &pos, uint32_t modifiers);

  /**
   * Start a new brush stroke (for drag operations).
   * Call endStroke() when drag completes.
   */
  void beginStroke();
  void beginStroke(uint32_t modifiers);
  void beginStroke(uint32_t modifiers, bool eraseMode);

  /**
   * Add a position to the current stroke.
   */
  void continueStroke(const Domain::Position &pos);

  /**
   * End the current stroke and push batch action.
   */
  void endStroke();

  bool refreshCurrentBrush();
  void cycleBrushVariation(int delta);
  int getBrushVariation() const { return variation_; }
  void setBrushVariation(int variation);
  void setBrushThickness(float thickness);
  float getBrushThickness() const;

  void adjustBrushSize(int delta);
  bool storeBrushSlot(size_t slot);
  bool recallBrushSlot(size_t slot);

  /**
   * Check if currently in a stroke.
   */
  bool isInStroke() const { return strokeActive_; }

  // Brush Size Control
  static constexpr int MIN_BRUSH_SIZE = 1;
  static constexpr int MAX_BRUSH_SIZE = 11;

  int getBrushSize() const { return brushSize_; }
  void setBrushSize(int size) {
    brushSize_ = std::clamp(size, MIN_BRUSH_SIZE, MAX_BRUSH_SIZE);
  }

  /**
   * Set the brush settings service for size/shape calculations.
   */
  void setBrushSettingsService(Services::BrushSettingsService *service) {
    brushSettingsService_ = service;
    // Wire settings service to spawn brush
    spawnBrush_.setSettingsService(service);
  }

  /**
   * Get the brush settings service.
   */
  Services::BrushSettingsService *getBrushSettingsService() const {
    return brushSettingsService_;
  }

  /**
   * Activate the spawn brush for placing spawn points.
   */
  void activateSpawnBrush();

  /**
   * Get the spawn brush instance.
   */
  SpawnBrush *getSpawnBrush() { return &spawnBrush_; }

  /**
   * Activate a flag brush for zone painting.
   */
  void activatePZBrush() { setBrush(&pzBrush_); }
  void activateNoPvpBrush() { setBrush(&noPvpBrush_); }
  void activateNoLogoutBrush() { setBrush(&noLogoutBrush_); }
  void activatePvpZoneBrush() { setBrush(&pvpZoneBrush_); }

  FlagBrush *getPZBrush() { return &pzBrush_; }
  FlagBrush *getNoPvpBrush() { return &noPvpBrush_; }
  FlagBrush *getNoLogoutBrush() { return &noLogoutBrush_; }
  FlagBrush *getPvpZoneBrush() { return &pvpZoneBrush_; }

  void activateEraserBrush() { setBrush(&eraserBrush_); }
  EraserBrush *getEraserBrush() { return &eraserBrush_; }

  void activateHouseBrush() { setBrush(&houseBrush_); }
  HouseBrush *getHouseBrush() { return &houseBrush_; }

  void activateHouseExitBrush() { setBrush(&houseExitBrush_); }
  HouseExitBrush *getHouseExitBrush() { return &houseExitBrush_; }

  void activateWaypointBrush() { setBrush(&waypointBrush_); }
  WaypointBrush *getWaypointBrush() { return &waypointBrush_; }

  void activateOptionalBorderBrush() { setBrush(&optionalBorderBrush_); }
  OptionalBorderBrush *getOptionalBorderBrush() { return &optionalBorderBrush_; }

  void activateNormalDoorBrush();
  void activateLockedDoorBrush();
  void activateQuestDoorBrush();
  void activateMagicDoorBrush();
  void activateArchwayBrush();
  void activateWindowBrush();
  void activateHatchWindowBrush();
  void activateNormalAltDoorBrush();

  DoorBrush *getNormalDoorBrush() { return normalDoorBrush_.get(); }
  DoorBrush *getNormalAltDoorBrush() { return normalAltDoorBrush_.get(); }
  DoorBrush *getLockedDoorBrush() { return lockedDoorBrush_.get(); }
  DoorBrush *getQuestDoorBrush() { return questDoorBrush_.get(); }
  DoorBrush *getMagicDoorBrush() { return magicDoorBrush_.get(); }
  DoorBrush *getArchwayBrush() { return archwayBrush_.get(); }
  DoorBrush *getWindowBrush() { return windowBrush_.get(); }
  DoorBrush *getHatchWindowBrush() { return hatchWindowBrush_.get(); }

  [[nodiscard]] bool isCurrentBrush(const IBrush *brush) const {
    return currentBrush_ == brush;
  }

private:
  Domain::ChunkedMap *map_ = nullptr;
  Domain::History::HistoryManager *historyManager_ = nullptr;
  Services::ClientDataService *clientData_ = nullptr;
  BrushRegistry *registry_ = nullptr;

  // Unified brush pointer
  IBrush *currentBrush_ = nullptr;
  std::string currentBrushName_;

  // Callback when brush is activated
  OnBrushActivatedCallback onBrushActivated_;
  TilesMutatedCallback onTilesMutated_;

  // Preview service for brush preview updates
  Services::Preview::PreviewService *previewService_ = nullptr;

  // Preview factory for creating preview providers
  Services::Preview::BrushPreviewFactory *previewFactory_ = nullptr;

  // Brush size (radius) - fallback if no BrushSettingsService
  int brushSize_ = 1;

  // Brush settings service for size/shape calculations
  Services::BrushSettingsService *brushSettingsService_ = nullptr;

  // Owned spawn brush instance
  SpawnBrush spawnBrush_;

  // Flag brush instances (one per flag type)
  FlagBrush pzBrush_{Domain::TileFlag::ProtectionZone, "PZ"};
  FlagBrush noPvpBrush_{Domain::TileFlag::NoPvp, "NoPvP"};
  FlagBrush noLogoutBrush_{Domain::TileFlag::NoLogout, "NoLogout"};
  FlagBrush pvpZoneBrush_{Domain::TileFlag::PvpZone, "PvPZone"};

  // Eraser brush instance
  EraserBrush eraserBrush_;

  // House brush instance
  HouseBrush houseBrush_;
  HouseExitBrush houseExitBrush_;

  // Waypoint brush instance
  WaypointBrush waypointBrush_;
  OptionalBorderBrush optionalBorderBrush_;
  std::unique_ptr<DoorBrush> normalDoorBrush_;
  std::unique_ptr<DoorBrush> normalAltDoorBrush_;
  std::unique_ptr<DoorBrush> lockedDoorBrush_;
  std::unique_ptr<DoorBrush> questDoorBrush_;
  std::unique_ptr<DoorBrush> magicDoorBrush_;
  std::unique_ptr<DoorBrush> archwayBrush_;
  std::unique_ptr<DoorBrush> windowBrush_;
  std::unique_ptr<DoorBrush> hatchWindowBrush_;

  Services::Autoborder::AutoborderEngine autoborderEngine_;

  // Simple flag for stroke tracking (HistoryManager handles actual undo)
  bool strokeActive_ = false;
  bool strokeEraseMode_ = false;
  bool strokeNeedsAutoborderFinalize_ = false;
  int variation_ = -1;
  uint32_t strokeModifiers_ = 0;

  std::array<std::optional<ResolvedBrushSelection>, 10> brushHotkeys_{};

  // Track positions painted in current stroke (to avoid duplicates)
  struct PositionHash {
    size_t operator()(const std::tuple<int32_t, int32_t, int16_t> &p) const {
      const uint64_t x = static_cast<uint64_t>(std::get<0>(p)) & 0xFFFFF; // 20 bits
      const uint64_t y = static_cast<uint64_t>(std::get<1>(p)) & 0xFFFFF; // 20 bits
      const uint64_t z = static_cast<uint64_t>(std::get<2>(p)) & 0xFFFF;  // 16 bits
      return std::hash<uint64_t>()(x | (y << 20) | (z << 40));
    }
  };
  std::unordered_set<std::tuple<int32_t, int32_t, int16_t>, PositionHash>
      paintedPositions_;

  // Track last position within current stroke for line interpolation
  std::optional<Domain::Position> lastStrokePos_;

  // Helper: get all positions on line between two points (Bresenham)
  std::vector<Domain::Position>
  getLinePositions(const Domain::Position &from,
                   const Domain::Position &to) const;

  enum class BrushActionFamily : uint8_t {
    GroundLike,
    WallLike,
    DoorLike,
    DoodadLike,
    PointLike,
  };

  [[nodiscard]] BrushActionFamily getActionFamily() const;
  [[nodiscard]] std::vector<Domain::Position>
  getBrushPositionsForCenter(const Domain::Position &center) const;
  [[nodiscard]] std::vector<Domain::Position> getPaintedStrokePositions() const;
  bool paintRecordedPosition(const Domain::Position &pos, uint32_t modifiers,
                             bool specialAction = false);
  bool paintRecordedPositions(std::span<const Domain::Position> positions,
                              uint32_t modifiers, bool specialAction = false);
  void eraseRecordedPosition(const Domain::Position &pos);
  void finalizeAutoborderStroke();
  void paintExpandedCenter(const Domain::Position &center, uint32_t modifiers);
  void eraseExpandedCenter(const Domain::Position &center);
  void continueGroundLikeStroke(const Domain::Position &pos);
  void continueWallLikeStroke(const Domain::Position &pos);
  void continueDoorLikeStroke(const Domain::Position &pos);
  void continueDoodadLikeStroke(const Domain::Position &pos);
  void continuePointLikeStroke(const Domain::Position &pos);
  void paintDoodadRecordedPosition(const Domain::Position &pos,
                                   uint32_t modifiers);
  void eraseDoodadRecordedPosition(const Domain::Position &pos,
                                   uint32_t modifiers);

  // Paint tile using current brush
  DrawContext createDrawContext(uint32_t modifiers,
                                bool specialAction = false) const;
  void paintTileDirect(const Domain::Position &pos, uint32_t modifiers,
                       bool specialAction = false);
  void notifyTilesMutated(const std::vector<Domain::Position> &positions) const;
  void resetStrokeState();
  [[nodiscard]] ResolvedBrushSelection captureCurrentSelection() const;
  bool applyResolvedSelection(const ResolvedBrushSelection &selection);
  DoorBrush *getDoorBrushForType(DoorType type) const;
  std::optional<ResolvedBrushSelection> lastBrushSelection_;

  // Brush resolution helper methods
  std::optional<ResolvedBrushSelection> makeSelection(const IBrush *brush, BrushPickMode selectionMode, std::string displayName = {}) const;
  std::optional<ResolvedBrushSelection> selectFlagBrush(Domain::TileFlag flag) const;
  std::optional<ResolvedBrushSelection> selectBrushById(BrushId brushId, BrushPickMode selectionMode) const;
  std::optional<ResolvedBrushSelection> selectPreferredBrushByType(BrushType type, BrushPickMode selectionMode, const Domain::Item *preferredItem) const;
  std::optional<ResolvedBrushSelection> selectDoorBrush(const Domain::Tile &tile, const Domain::Item *preferredItem) const;
  std::optional<ResolvedBrushSelection> selectGroundBrush(const Domain::Tile &tile) const;
  std::optional<ResolvedBrushSelection> selectRawBrush(const Domain::Tile &tile, const Domain::Item *preferredItem) const;
  std::optional<ResolvedBrushSelection> selectHouseExitBrush(const Domain::Tile &tile) const;
  std::optional<ResolvedBrushSelection> selectWaypointBrush(const Domain::Tile &tile) const;
  std::optional<ResolvedBrushSelection> selectOptionalBorderBrush(const Domain::Tile &tile) const;
  std::optional<ResolvedBrushSelection> selectCollectionBrush(const Domain::Tile &tile, const Domain::Item *preferredItem) const;
  std::optional<ResolvedBrushSelection> selectSpecificMode(const Domain::Tile &tile, BrushPickMode pickMode, const Domain::Item *preferredItem) const;

  // Alt-replace state for GroundBrush (per-stroke, replaces global)
  mutable DrawContext::AltGroundReplaceState altReplaceState_;
};

} // namespace MapEditor::Brushes
