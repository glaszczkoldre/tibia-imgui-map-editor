#pragma once
#include "Domain/CopyBuffer.h"
#include "Domain/MapInstance.h"
#include "Rendering/Frame/RenderingManager.h" // For SessionID
#include "Rendering/Selection/SelectionDataProviderAdapter.h"
#include "Services/CreatureSimulator.h"
#include "Services/Preview/PreviewService.h"
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

namespace MapEditor::Services {
class ClientDataService;
}

namespace MapEditor::AppLogic {

/**
 * Per-map editing session.
 * Bundles map data + selection state + view state + undo history.
 * One EditorSession per open tab.
 */
class EditorSession {
public:
  using SessionID = MapEditor::Rendering::RenderingManager::SessionID;

  explicit EditorSession(std::unique_ptr<Domain::MapInstance> document,
                         SessionID session_id);
  ~EditorSession();

  // Non-copyable, movable
  EditorSession(const EditorSession &) = delete;
  EditorSession &operator=(const EditorSession &) = delete;
  EditorSession(EditorSession &&) = default;
  EditorSession &operator=(EditorSession &&) = default;

  // Session ID
  SessionID getID() const { return session_id_; }

  // Document access
  Domain::MapInstance *getDocument() { return document_.get(); }
  const Domain::MapInstance *getDocument() const { return document_.get(); }

  // Map access (Delegated)
  Domain::ChunkedMap *getMap() { return document_->getMap(); }
  const Domain::ChunkedMap *getMap() const { return document_->getMap(); }

  // Selection access (Delegated to SelectionService)
  Services::Selection::SelectionService &getSelectionService() {
    return document_->getSelectionService();
  }
  const Services::Selection::SelectionService &getSelectionService() const {
    return document_->getSelectionService();
  }

  /**
   * Get selection data provider for rendering layer.
   * Returns adapter that implements ISelectionDataProvider interface.
   * This follows Dependency Inversion: rendering depends on abstraction.
   */
  const Rendering::ISelectionDataProvider *getSelectionProvider() const {
    return &selection_adapter_;
  }

  // Selection operations (Delegated)
  void selectRegion(int32_t min_x, int32_t min_y, int32_t max_x, int32_t max_y,
                    int16_t z) {
    document_->selectRegion(min_x, min_y, max_x, max_y, z);
  }
  void clearSelection() { document_->clearSelection(); }
  void deleteSelection() { document_->deleteSelection(); }

  // Undo/Redo (Delegated)
  bool canUndo() const { return document_->canUndo(); }
  bool canRedo() const { return document_->canRedo(); }
  std::string undo() { return document_->undo(); }
  std::string redo() { return document_->redo(); }
  Domain::History::HistoryManager &getHistoryManager() {
    return document_->getHistoryManager();
  }

  // Metadata (Delegated)
  const std::filesystem::path &getFilePath() const {
    return document_->getFilePath();
  }
  void setFilePath(const std::filesystem::path &path) {
    document_->setFilePath(path);
  }
  std::string getDisplayName() const { return document_->getDisplayName(); }

  // Dirty state (Delegated)
  bool isModified() const { return document_->isModified(); }
  void setModified(bool modified) {
    document_->setModified(modified);
  } // Callback handled in Document

  // Modified callback (Delegated)
  using ModifiedCallback = Domain::MapInstance::ModifiedCallback;
  void setOnModifiedCallback(ModifiedCallback cb) {
    document_->setOnModifiedCallback(std::move(cb));
  }

  // RME-style Paste Preview Mode
  bool isPasting() const { return is_pasting_; }
  bool isPasteReplaceMode() const { return paste_replace_mode_; }
  void startPaste(const std::vector<Domain::CopyBuffer::CopiedTile>
                      &tiles, bool replace_mode = false); // Clones tiles for preview
  void cancelPaste();
  void confirmPaste(const Domain::Position &target_pos,
                    bool replace_mode = false); // Commits paste action

  // View state (preserved when switching tabs)
  struct ViewState {
    float camera_x = 0.0f;
    float camera_y = 0.0f;
    float zoom = 1.0f;
    int current_floor = 7;
    // Per-map lighting settings
    bool lighting_enabled = false;
    int ambient_light = 128;
    // Per-map window visibility
    bool show_ingame_box = false;
    bool show_minimap = false;
  };
  ViewState &getViewState() { return view_state_; }
  const ViewState &getViewState() const { return view_state_; }

  // Minimap state (preserved when switching tabs)
  struct MinimapState {
    int32_t center_x = 0;
    int32_t center_y = 0;
    int16_t floor = 7;
    int zoom_level = 2; // 1:1=0, 1:2=1, 1:4=2, 1:8=3, 1:16=4
  };
  MinimapState &getMinimapState() { return minimap_state_; }
  const MinimapState &getMinimapState() const { return minimap_state_; }

  // Ingame preview state (preserved when switching tabs)
  struct IngamePreviewState {
    bool is_open = false;
    bool follow_cursor = true;
    int32_t locked_x = 0;
    int32_t locked_y = 0;
    int16_t locked_z = 7;
    // Configurable preview dimensions
    int width_tiles = 15;
    int height_tiles = 11;
  };
  IngamePreviewState &getIngamePreviewState() { return ingame_preview_state_; }
  const IngamePreviewState &getIngamePreviewState() const {
    return ingame_preview_state_;
  }

  // Browse Tile state (preserved when switching tabs)
  struct BrowseTileState {
    bool is_open = false;
  };
  BrowseTileState &getBrowseTileState() { return browse_tile_state_; }
  const BrowseTileState &getBrowseTileState() const {
    return browse_tile_state_;
  }

  // NOTE: getRenderState() removed. Pass SessionID to RenderOrchestrator
  // instead.

  // Logic state
  Services::CreatureSimulator &getCreatureSimulator() {
    return creature_simulator_;
  }

  // Preview service (centralized preview system)
  Services::Preview::PreviewService &getPreviewService() {
    return preview_service_;
  }
  const Services::Preview::PreviewService &getPreviewService() const {
    return preview_service_;
  }

private:
  std::unique_ptr<Domain::MapInstance> document_;
  SessionID session_id_;

  // Paste Preview State
  bool is_pasting_ = false;
  bool paste_replace_mode_ = false;  // True if Ctrl+Shift+V was used
  std::vector<Domain::CopyBuffer::CopiedTile> paste_preview_;

  ViewState view_state_;
  MinimapState minimap_state_;
  IngamePreviewState ingame_preview_state_;
  BrowseTileState browse_tile_state_;

  // RenderState is now managed by RenderingManager via SessionID

  // Logic state
  Services::CreatureSimulator creature_simulator_;

  // Preview service
  Services::Preview::PreviewService preview_service_;

  // Selection adapter for rendering (wraps SelectionService to ISelectionDataProvider)
  // Mutable because it's a lazy-initialized cache that updates its service pointer
  mutable Rendering::SelectionDataProviderAdapter selection_adapter_;
};

} // namespace MapEditor::AppLogic
