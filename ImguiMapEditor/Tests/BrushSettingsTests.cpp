#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Services/BrushSettingsService.h"
#include "UI/Helpers/BrushToolOptionsModel.h"
#include "UI/Utils/IconTextureCache.h"
#include "Brushes/Core/IBrush.h"

namespace fs = std::filesystem;

namespace {

void require(bool condition, const std::string &msg) {
  if (!condition) {
    throw std::runtime_error(msg);
  }
}

// Minimal implementation of IBrush to test visibility and capability options
class DummyBrush : public MapEditor::Brushes::IBrush {
public:
  DummyBrush(MapEditor::Brushes::BrushType type, bool needBorders = false)
      : type_(type), needBorders_(needBorders) {}

  const std::string &getName() const override {
    static std::string name = "Dummy";
    return name;
  }

  MapEditor::Brushes::BrushType getType() const override { return type_; }
  uint32_t getLookId() const override { return 0; }
  bool needBorders() const override { return needBorders_; }

  void draw(MapEditor::Domain::ChunkedMap &map, MapEditor::Domain::Tile *tile,
            const MapEditor::Brushes::DrawContext &ctx) override {}
  void undraw(MapEditor::Domain::ChunkedMap &map,
              MapEditor::Domain::Tile *tile) override {}

private:
  MapEditor::Brushes::BrushType type_;
  bool needBorders_;
};

void testBrushSettingsServiceSizeAndFootprints() {
  MapEditor::Services::BrushSettingsService service;

  // 1. Test exact size mode
  service.setExactBrushSize(true);
  require(service.isExactBrushSize(), "Exact size mode should be active");
  service.setBrushSizeAxes(3, 4);
  require(service.getBrushSizeX() == 3, "Size X should be 3");
  require(service.getBrushSizeY() == 4, "Size Y should be 4");
  require(service.getEffectiveAxisSpanX() == 3, "Exact mode X span should be 3");
  require(service.getEffectiveAxisSpanY() == 4, "Exact mode Y span should be 4");

  // 2. Test radius size mode
  service.setExactBrushSize(false);
  require(!service.isExactBrushSize(), "Radius size mode should be active");
  service.setBrushSizeAxes(2, 3);
  require(service.getBrushSizeX() == 2, "Size X should be 2");
  require(service.getBrushSizeY() == 3, "Size Y should be 3");
  require(service.getEffectiveAxisSpanX() == 5, "Radius mode X span should be 2*2+1 = 5");
  require(service.getEffectiveAxisSpanY() == 7, "Radius mode Y span should be 3*2+1 = 7");

  // 3. Test aspect lock behavior
  service.setBrushAspectRatioLocked(true);
  require(service.isBrushAspectRatioLocked(), "Aspect lock should be active");
  service.setBrushSizeX(5);
  require(service.getBrushSizeY() == 5, "Aspect lock should sync Y with X");
  service.setBrushSizeY(8);
  require(service.getBrushSizeX() == 8, "Aspect lock should sync X with Y");

  // 4. Test Square vs Circle footprint containment
  service.setExactBrushSize(false);
  service.setBrushShape(MapEditor::Services::BrushShape::Square);
  service.setBrushSizeAxes(1, 1);
  auto squareOffsets = service.getBrushOffsets();
  require(squareOffsets.size() == 9, "Square footprint size 1 should have 9 tiles (3x3)");

  service.setBrushShape(MapEditor::Services::BrushShape::Circle);
  auto circleOffsets = service.getBrushOffsets();
  // Ellipse containment verification:
  // dx^2 / rx^2 + dy^2 / ry^2 <= 1.005. Here rx=1, ry=1.
  // offsets (dx, dy):
  // (0,0) -> 0 <= 1 (yes)
  // (1,0), (-1,0), (0,1), (0,-1) -> 1 <= 1 (yes)
  // (1,1), (-1,1), (1,-1), (-1,-1) -> 2 <= 1 (no)
  // So circle footprint should contain exactly 5 offsets.
  require(circleOffsets.size() == 5, "Circle footprint size 1 should contain 5 tiles");

  // 5. Test legacy size sequence stepping
  // Progression: 0, 1, 2, 4, 6, 8, 11
  service.setStandardSize(2);
  require(service.getStandardSize() == 2, "Standard size should be 2");
  service.increaseSize();
  require(service.getStandardSize() == 4, "Standard size increased should be 4");
  service.increaseSize();
  require(service.getStandardSize() == 6, "Standard size increased should be 6");
  service.decreaseSize();
  require(service.getStandardSize() == 4, "Standard size decreased should be 4");

  // 6. Test spawn forcing square footprint
  service.setBrushShape(MapEditor::Services::BrushShape::Circle);
  service.setBrushSizeAxes(1, 1);
  DummyBrush spawnBrush(MapEditor::Brushes::BrushType::Spawn);
  auto spawnOffsets = service.getBrushOffsets(true); // forceSquare = true for spawn
  require(spawnOffsets.size() == 9, "Spawn brush footprint should be square (9 offsets)");
}

void testVisibilityHelpers() {
  DummyBrush ground(MapEditor::Brushes::BrushType::Ground, true);
  DummyBrush waypoint(MapEditor::Brushes::BrushType::Waypoint);
  DummyBrush door(MapEditor::Brushes::BrushType::Door);
  DummyBrush doodad(MapEditor::Brushes::BrushType::Doodad);
  DummyBrush spawn(MapEditor::Brushes::BrushType::Spawn);

  // 1. waypoint/houseexit hides size
  require(!MapEditor::UI::BrushToolOptionsModel::hasBrushSizeControls(&waypoint),
          "Waypoint should hide size controls");
  require(MapEditor::UI::BrushToolOptionsModel::hasBrushSizeControls(&ground),
          "Ground should show size controls");

  // 2. doodad shows thickness
  require(MapEditor::UI::BrushToolOptionsModel::hasThicknessControl(&doodad),
          "Doodad should show thickness");
  require(!MapEditor::UI::BrushToolOptionsModel::hasThicknessControl(&ground),
          "Ground should hide thickness");

  // 3. needBorders() shows preview border
  require(MapEditor::UI::BrushToolOptionsModel::hasPreviewBorderControl(&ground),
          "Ground brush (needBorders=true) should show preview border");
  require(!MapEditor::UI::BrushToolOptionsModel::hasPreviewBorderControl(&waypoint),
          "Waypoint (needBorders=false) should hide preview border");

  // 4. door shows lock doors
  require(MapEditor::UI::BrushToolOptionsModel::hasLockDoorsControl(&door),
          "Door should show lock doors option");
  require(!MapEditor::UI::BrushToolOptionsModel::hasLockDoorsControl(&ground),
          "Ground should hide lock doors option");

  // 5. creature/spawn shows spawn options and isCreatureToolMode
  require(MapEditor::UI::BrushToolOptionsModel::isCreatureToolMode(&spawn),
          "Spawn brush should be in creature tool mode");
  require(MapEditor::UI::BrushToolOptionsModel::hasSpawnControls(&spawn),
          "Spawn brush should show spawn options");
  require(!MapEditor::UI::BrushToolOptionsModel::isCreatureToolMode(&ground),
          "Ground brush should not be in creature tool mode");
}

void testPNGIconCacheLoading() {
  // Setup a headless OpenGL context to test the texture loading
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  GLFWwindow *window =
      glfwCreateWindow(100, 100, "Headless Test Context", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    throw std::runtime_error("Failed to create headless GLFW window");
  }

  glfwMakeContextCurrent(window);
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    glfwDestroyWindow(window);
    glfwTerminate();
    throw std::runtime_error("Failed to initialize GLAD loader");
  }

  try {
    MapEditor::UI::IconTextureCache cache;
    // Resolve absolute path using the CMake compile definition macro
    fs::path projectRoot = BRUSH_SETTINGS_TESTS_SOURCE_DIR;
    fs::path assetsDir = projectRoot.parent_path() / "assets" / "png";

    std::cout << "[Test] Loading assets from resolved dir: " << assetsDir.string() << std::endl;
    cache.loadAll(assetsDir.string());

    // Verify some loaded icons
    auto eraserInfo = cache.getIcon("eraser");
    require(eraserInfo.textureId != ImTextureID{}, "Eraser icon texture should be loaded");
    require(eraserInfo.width > 0 && eraserInfo.height > 0, "Eraser icon dimensions should be > 0");

    auto pzInfo = cache.getIcon("protection_zone");
    require(pzInfo.textureId != ImTextureID{}, "PZ icon texture should be loaded");

    // Clean up context
    glfwDestroyWindow(window);
    glfwTerminate();
    std::cout << "PNG Icon Cache tests OK" << std::endl;
  } catch (const std::exception &ex) {
    glfwDestroyWindow(window);
    glfwTerminate();
    throw;
  }
}

} // namespace

int main() {
  try {
    std::cout << "Running BrushSettings unit tests..." << std::endl;

    testBrushSettingsServiceSizeAndFootprints();
    std::cout << "BrushSettingsService size and footprints tests OK" << std::endl;

    testVisibilityHelpers();
    std::cout << "Brush visibility and model helper tests OK" << std::endl;

    testPNGIconCacheLoading();

    std::cout << "All BrushSettings tests passed successfully!" << std::endl;
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "Test suite failed: " << ex.what() << std::endl;
    return 1;
  }
}
