#pragma once
#include "Core/Config.h"

namespace MapEditor {
namespace Rendering {

/**
 * LOD (Level of Detail) Policy Template.
 *
 * Defines the rules for "LOD Mode".
 * This class does NOT hold state. It answers: "If LOD is active, what should
 * happen?"
 *
 * The active state is determined by the Zoom Threshold in
 * Config::Rendering::LOD::THRESHOLD, but the APPLICATION APPLICATION (MapPanel)
 * is responsible for detecting this and pushing the state to managers
 * (OverlayManager, MapRenderer).
 */
class LODPolicy {
public:
  /**
   * Helper to check if LOD mode *should* be active for a given zoom.
   * Used by MapPanel to detect state transitions.
   */
  static bool isLodActive(float zoom) {
    return zoom < Config::Rendering::LOD::THRESHOLD;
  }

  // ==============================================================================
  // RULES TEMPLATE (What happens when LOD is ACTIVE?)
  // ==============================================================================

  // Should we show text names above creatures?
  static constexpr bool SHOW_CREATURE_NAMES = false;

  // Should we show name labels above static outfits?
  static constexpr bool SHOW_OUTFIT_NAMES = false;

  // Should we show "SPAWN" text labels?
  static constexpr bool SHOW_SPAWN_LABELS = false;

  // Should we show hover tooltips?
  static constexpr bool SHOW_TOOLTIPS = false;

  // Should creatures animate?
  static constexpr bool ANIMATE_CREATURES = false;

  // Should we use batched rendering (optimization for zoomed-out view)?
  static constexpr bool USE_BATCHING = true;
};

} // namespace Rendering
} // namespace MapEditor
