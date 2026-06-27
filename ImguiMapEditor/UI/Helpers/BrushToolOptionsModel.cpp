#include "BrushToolOptionsModel.h"

#include "Brushes/Core/IBrush.h"

namespace MapEditor::UI {

bool BrushToolOptionsModel::isCreatureToolMode(const Brushes::IBrush *brush) {
  if (!brush) {
    return false;
  }
  return brush->getType() == Brushes::BrushType::Creature ||
         brush->getType() == Brushes::BrushType::Spawn;
}

bool BrushToolOptionsModel::hasBrushSizeControls(const Brushes::IBrush *brush) {
  if (!brush || hasSpawnControls(brush)) {
    return false;
  }
  return brush->getType() != Brushes::BrushType::Waypoint &&
         brush->getType() != Brushes::BrushType::HouseExit;
}

bool BrushToolOptionsModel::hasThicknessControl(const Brushes::IBrush *brush) {
  if (!brush) {
    return false;
  }
  return brush->getType() == Brushes::BrushType::Doodad;
}

bool BrushToolOptionsModel::hasPreviewBorderControl(const Brushes::IBrush *brush) {
  if (!brush) {
    return false;
  }
  return brush->needBorders();
}

bool BrushToolOptionsModel::hasAutoBorderControl(const Brushes::IBrush *brush) {
  if (!brush) {
    return false;
  }
  return brush->needBorders();
}

bool BrushToolOptionsModel::hasLockDoorsControl(const Brushes::IBrush *brush) {
  if (!brush) {
    return false;
  }
  return brush->getType() == Brushes::BrushType::Door;
}

bool BrushToolOptionsModel::hasSpawnControls(const Brushes::IBrush *brush) {
  return isCreatureToolMode(brush);
}

} // namespace MapEditor::UI
