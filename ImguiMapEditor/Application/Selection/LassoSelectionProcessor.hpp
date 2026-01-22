#pragma once
#include "Domain/ICoordinateTransformer.h"
#include "Domain/Position.h"
#include "Domain/SelectionSettings.h"
#include <glm/vec2.hpp>
#include <span>

namespace MapEditor {
namespace AppLogic {
class EditorSession;
}

namespace Application {
namespace Selection {

/**
 * Handles the business logic for Lasso Selection.
 * Calculates which tiles fall within a user-drawn polygon and applies selection.
 */
class LassoSelectionProcessor {
public:
  // Enum to define selection behavior explicitly
  enum class SelectionMode {
    Replace, // Clear existing and select new
    Add,     // Add new to existing (Union)
    Subtract // Remove new from existing (Difference)
  };

  /**
   * Processes the lasso selection.
   *
   * @param session The editor session (for SelectionService access).
   * @param camera The coordinate transformer (decoupled from concrete Camera).
   * @param selection_settings Settings determining floor scope.
   * @param polygon_points The screen coordinates of the lasso polygon.
   * @param mode The selection mode (Replace, Add, Subtract).
   */
  static void process(AppLogic::EditorSession *session,
                      const Domain::ICoordinateTransformer &camera,
                      const Domain::SelectionSettings *selection_settings,
                      std::span<const glm::vec2> polygon_points,
                      SelectionMode mode);

private:
  static bool isPointInPolygon(const glm::vec2 &point,
                               std::span<const glm::vec2> polygon);
};

} // namespace Selection
} // namespace Application
} // namespace MapEditor
