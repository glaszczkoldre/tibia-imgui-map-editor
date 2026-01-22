#pragma once
#include "PropertyWidgets.h"
#include <cstdint>

namespace MapEditor {
namespace Domain {
class Item;
class ItemType;
struct Spawn;
struct Creature;
class ChunkedMap;
} // namespace Domain
namespace Services {
class SpriteManager;
}
} // namespace MapEditor

namespace MapEditor::UI::Properties {

/**
 * Panel types for the property renderer.
 */
enum class PanelType {
  None,
  Container,
  Writeable,
  Splash,
  Depot,
  Door,
  Teleport,
  Podium,
  NormalItem,
  Spawn,
  Creature
};

/**
 * Single renderer class that dynamically displays properties based on
 * item/spawn/creature type. Uses internal type switching and shared
 * PropertyWidgets for DRY compliance.
 *
 * Auto-applies changes on edit with visual feedback (green border flash).
 */
class PropertyPanelRenderer {
public:
  /**
   * Set the context for rendering.
   * @param item Item to edit (nullptr if none)
   * @param spawn Spawn to edit (nullptr if none)
   * @param creature Creature to edit (nullptr if none)
   * @param otbm_version Map OTBM version for feature gating
   * @param sprite_manager For container item sprites (optional)
   * @param map For town lookup in depot dropdown (optional)
   */
  void setContext(Domain::Item *item, Domain::Spawn *spawn,
                  Domain::Creature *creature, uint32_t otbm_version,
                  Services::SpriteManager *sprite_manager = nullptr,
                  uint16_t map_width = 65535, uint16_t map_height = 65535,
                  Domain::ChunkedMap *map = nullptr);

  /**
   * Render the appropriate property panel.
   * Auto-applies changes and shows visual feedback.
   */
  void render();

  /**
   * Check if there are pending changes.
   */
  bool hasChanges() const { return dirty_; }

  /**
   * Get current panel type.
   */
  PanelType getCurrentPanelType() const { return panel_type_; }

  /**
   * Get display name for current panel.
   */
  const char *getPanelName() const;

private:
  PanelType detectPanelType() const;
  void loadValuesFromContext();
  void applyChangesToContext();

  // Section renderers
  void renderNormalSection();
  void renderContainerSection();
  void renderWriteableSection();
  void renderSplashSection();
  void renderDepotSection();
  void renderDoorSection();
  void renderTeleportSection();
  void renderPodiumSection();
  void renderSpawnSection();
  void renderCreatureSection();
  void renderCommonItemFields();
  void renderApplyIndicator();

  // Context (non-owning)
  Domain::Item *item_ = nullptr;
  Domain::Spawn *spawn_ = nullptr;
  Domain::Creature *creature_ = nullptr;
  const Domain::ItemType *item_type_ = nullptr;
  Services::SpriteManager *sprite_manager_ = nullptr;
  uint32_t otbm_version_ = 0;
  uint16_t map_width_ = 65535;
  uint16_t map_height_ = 65535;
  Domain::ChunkedMap *map_ = nullptr;
  PanelType panel_type_ = PanelType::None;

  // Track context changes
  Domain::Item *last_item_ = nullptr;
  Domain::Spawn *last_spawn_ = nullptr;
  Domain::Creature *last_creature_ = nullptr;

  // Editable state (copied from objects)
  struct EditState {
    int action_id = 0;
    int unique_id = 0;
    int count = 1;
    int tier = 0;
    int charges = 0;
    int door_id = 0;
    int depot_id = 0;
    int fluid_type = 0;
    int tele_x = 0, tele_y = 0, tele_z = 0;
    int direction = 0;
    int spawn_radius = 1;
    int spawn_time = 60;
    char text[4096] = {0};
    OutfitEdit outfit;
    bool show_outfit = true;
    bool show_mount = true;
    bool show_platform = true;
  } edit_;

  bool dirty_ = false;
  bool just_applied_ = false;
  int apply_flash_frames_ = 0;
};

} // namespace MapEditor::UI::Properties
