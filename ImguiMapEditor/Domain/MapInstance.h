#pragma once
#include "ChunkedMap.h"
#include "History/HistoryManager.h"
#include "Services/Selection/SelectionService.h"
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

namespace MapEditor::Services {
class ClientDataService;
}

namespace MapEditor::Domain {

/**
 * Represents the pure domain data of a map project.
 * Decoupled from rendering context (OpenGL) and view state.
 *
 * Responsibilities:
 * - Holds the map data (ChunkedMap)
 * - Manages selection state via SelectionService
 * - Manages undo/redo history
 * - Tracks file path and modification state
 */
class MapInstance {
public:
  explicit MapInstance(std::unique_ptr<ChunkedMap> map,
                       Services::ClientDataService *client_data = nullptr);
  ~MapInstance();

  // Non-copyable, movable
  MapInstance(const MapInstance &) = delete;
  MapInstance &operator=(const MapInstance &) = delete;
  MapInstance(MapInstance &&) = default;
  MapInstance &operator=(MapInstance &&) = default;

  // Map Access
  ChunkedMap *getMap() { return map_.get(); }
  const ChunkedMap *getMap() const { return map_.get(); }

  // Selection Access - using new SelectionService
  Services::Selection::SelectionService &getSelectionService() {
    return selection_service_;
  }
  const Services::Selection::SelectionService &getSelectionService() const {
    return selection_service_;
  }

  // Selection Operations
  void selectRegion(int32_t min_x, int32_t min_y, int32_t max_x, int32_t max_y,
                    int16_t z);
  void clearSelection();
  void deleteSelection();

  // History Operations
  bool canUndo() const { return history_manager_.canUndo(); }
  bool canRedo() const { return history_manager_.canRedo(); }
  std::string undo();
  std::string redo();
  History::HistoryManager &getHistoryManager() { return history_manager_; }

  // Metadata
  const std::filesystem::path &getFilePath() const { return file_path_; }
  void setFilePath(const std::filesystem::path &path);
  std::string getDisplayName() const;

  // Dirty State
  bool isModified() const { return modified_; }
  void setModified(bool modified);

  // Callback for modification (e.g. for UI update or auto-save)
  using ModifiedCallback = std::function<void(bool)>;
  void setOnModifiedCallback(ModifiedCallback cb) {
    on_modified_callback_ = std::move(cb);
  }

  // Services
  Services::ClientDataService *getClientData() const { return client_data_; }

private:
  std::unique_ptr<ChunkedMap> map_;
  Services::Selection::SelectionService selection_service_;
  History::HistoryManager history_manager_;
  Services::ClientDataService *client_data_ = nullptr;

  std::filesystem::path file_path_;
  bool modified_ = false;
  ModifiedCallback on_modified_callback_;
};

} // namespace MapEditor::Domain
