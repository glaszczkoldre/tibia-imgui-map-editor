#include "Brushes/BrushRegistry.h"
#include "Brushes/Types/DoodadBrush.h"
#include "IO/DoodadXmlHelper.h"
#include <pugixml.hpp>
#include <iostream>
#include <stdexcept>
#include <cassert>

namespace {
void require(bool condition, const std::string &msg) {
  if (!condition) {
    throw std::runtime_error(msg);
  }
}
} // namespace

int main() {
  try {
    // 1. Test default draggable is false
    {
      MapEditor::Brushes::BrushRegistry registry;
      pugi::xml_document doc;
      require(doc.load_string("<brush type=\"doodad\"><item id=\"1000\" chance=\"10\"/></brush>"), "xml load failed");
      
      MapEditor::IO::parseDoodadBrush(doc.child("brush"), "default_drag", 1000, "test.xml", registry);
      auto *brush = registry.getBrush("default_drag");
      require(brush != nullptr, "failed to parse brush");
      require(!brush->isDraggable(), "default draggable should be false");
    }

    // 2. Test explicit draggable is true/false
    {
      MapEditor::Brushes::BrushRegistry registry;
      pugi::xml_document doc;
      require(doc.load_string("<brush type=\"doodad\" draggable=\"true\"><item id=\"1000\" chance=\"10\"/></brush>"), "xml load failed");
      
      MapEditor::IO::parseDoodadBrush(doc.child("brush"), "explicit_drag", 1000, "test.xml", registry);
      auto *brush = registry.getBrush("explicit_drag");
      require(brush != nullptr, "failed to parse brush");
      require(brush->isDraggable(), "explicit draggable true failed");
    }

    // 3. Test chance attribute validation (negative clamped to 0, missing/malformed skipped)
    {
      MapEditor::Brushes::BrushRegistry registry;
      pugi::xml_document doc;
      require(doc.load_string(
        "<brush type=\"doodad\">"
        "  <item id=\"1000\" chance=\"10\"/>" // valid
        "  <item id=\"1001\" chance=\"-5\"/>" // negative -> clamped to 0
        "  <item id=\"1002\"/>"               // missing chance -> skipped
        "  <item id=\"1003\" chance=\"abc\"/>" // malformed chance -> skipped
        "</brush>"
      ), "xml load failed");
      
      MapEditor::IO::parseDoodadBrush(doc.child("brush"), "chance_test", 1000, "test.xml", registry);
      auto *brush = dynamic_cast<MapEditor::Brushes::DoodadBrush*>(registry.getBrush("chance_test"));
      require(brush != nullptr, "failed to parse brush");
      
      const auto *alt = brush->getAlternative(0);
      require(alt != nullptr, "alternative missing");
      const auto &singles = alt->getSingleItems();
      
      // Should only have 2 single items: 1000 and 1001
      require(singles.size() == 2, "chance test: singles count mismatch: " + std::to_string(singles.size()));
      require(singles[0].itemId == 1000 && singles[0].chance == 10, "item 1000 invalid");
      require(singles[1].itemId == 1001 && singles[1].chance == 0, "item 1001 invalid (should be clamped to 0)");
    }

    // 4. Test composite tile range checks (dx/dy in [-32,32], dz in [-15,15], skip empty composites)
    {
      MapEditor::Brushes::BrushRegistry registry;
      pugi::xml_document doc;
      require(doc.load_string(
        "<brush type=\"doodad\">"
        "  <composite chance=\"10\">"
        "    <tile x=\"5\" y=\"5\" z=\"1\"><item id=\"100\"/></tile>"   // valid
        "    <tile x=\"40\" y=\"5\"><item id=\"101\"/></tile>"          // invalid x (40 > 32)
        "    <tile x=\"5\" y=\"-33\"><item id=\"102\"/></tile>"         // invalid y (-33 < -32)
        "    <tile x=\"5\" y=\"5\" z=\"16\"><item id=\"103\"/></tile>"  // invalid z (16 > 15)
        "    <tile x=\"abc\" y=\"5\"><item id=\"104\"/></tile>"         // malformed x
        "    <tile y=\"5\"><item id=\"105\"/></tile>"                   // missing x
        "  </composite>"
        "  <composite chance=\"20\">" // empty composite (no valid tiles) -> skipped
        "    <tile x=\"45\" y=\"0\"><item id=\"106\"/></tile>"
        "  </composite>"
        "</brush>"
      ), "xml load failed");
      
      MapEditor::IO::parseDoodadBrush(doc.child("brush"), "composite_test", 1000, "test.xml", registry);
      auto *brush = dynamic_cast<MapEditor::Brushes::DoodadBrush*>(registry.getBrush("composite_test"));
      require(brush != nullptr, "failed to parse brush");
      
      const auto *alt = brush->getAlternative(0);
      require(alt != nullptr, "alternative missing");
      const auto &composites = alt->getComposites();
      
      // Should have exactly 1 composite (the first one)
      require(composites.size() == 1, "composites count mismatch: " + std::to_string(composites.size()));
      require(composites[0].chance == 10, "composite chance invalid");
      
      // The composite should have exactly 1 valid tile (the first tile)
      require(composites[0].tiles.size() == 1, "composite tiles count mismatch: " + std::to_string(composites[0].tiles.size()));
      require(composites[0].tiles[0].dx == 5 && composites[0].tiles[0].dy == 5 && composites[0].tiles[0].dz == 1, "composite tile offset invalid");
    }

    std::cout << "DoodadXmlSmoke passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "DoodadXmlSmoke failed: " << e.what() << "\n";
    return 1;
  }
}
