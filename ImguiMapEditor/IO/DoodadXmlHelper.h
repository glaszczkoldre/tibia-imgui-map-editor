#pragma once

#include <filesystem>
#include <pugixml.hpp>
#include <string>

namespace MapEditor::Brushes {
class BrushRegistry;
}

namespace MapEditor::IO {

void parseDoodadBrush(const pugi::xml_node &node,
                      const std::string &name,
                      uint32_t lookId,
                      const std::filesystem::path &sourceFile,
                      Brushes::BrushRegistry &brushRegistry);

} // namespace MapEditor::IO
