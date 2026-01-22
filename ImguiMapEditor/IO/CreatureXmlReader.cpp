#include "CreatureXmlReader.h"
#include "XmlUtils.h"
#include <spdlog/spdlog.h>

namespace MapEditor::IO {

CreatureXmlResult CreatureXmlReader::read(const std::filesystem::path &path) {
  CreatureXmlResult result;
  pugi::xml_document doc;

  pugi::xml_node rootNode =
      XmlUtils::loadXmlFile(path, "creatures", doc, result.error);
  if (!rootNode) {
    return result;
  }

  // Iterate over all children of the root node to find sections
  // This is more robust than child("name") if there are ordering/structure
  // surprises
  for (pugi::xml_node child : rootNode) {
    std::string nodeName = child.name();

    if (nodeName == "monsters") {
      for (pugi::xml_node monsterNode : child.children("monster")) {
        auto creature = parseCreatureNode(monsterNode, false, result.warnings);
        if (creature) {
          result.creatures.push_back(std::move(creature));
        }
      }
    } else if (nodeName == "npcs") {
      for (pugi::xml_node npcNode : child.children("npc")) {
        auto creature = parseCreatureNode(npcNode, true, result.warnings);
        if (creature) {
          result.creatures.push_back(std::move(creature));
        }
      }
    } else if (nodeName == "creature") {
      // Handle flat structure where <creature> nodes are direct children
      // Check the "type" attribute to determine if it's an NPC
      bool isNpc = false;
      if (auto typeAttr = child.attribute("type")) {
        std::string typeStr = typeAttr.as_string();
        isNpc = (typeStr == "npc" || typeStr == "NPC");
      }
      auto creature = parseCreatureNode(child, isNpc, result.warnings);
      if (creature) {
        result.creatures.push_back(std::move(creature));
      }
    } else if (nodeName == "npc") {
      // Handle direct <npc> children (flat structure without <npcs> wrapper)
      auto creature = parseCreatureNode(child, true, result.warnings);
      if (creature) {
        result.creatures.push_back(std::move(creature));
      }
    } else if (nodeName == "monster") {
      // Handle direct <monster> children (flat structure without <monsters>
      // wrapper)
      auto creature = parseCreatureNode(child, false, result.warnings);
      if (creature) {
        result.creatures.push_back(std::move(creature));
      }
    }
  }

  result.success = true;
  spdlog::info(
      "[CreatureXmlReader] Loaded {} creatures from XML (warnings: {})",
      result.creatures.size(), result.warnings.size());

  // Log first few warnings if any
  for (size_t i = 0; i < std::min(result.warnings.size(), size_t(5)); ++i) {
    spdlog::warn("[CreatureXmlReader] {}", result.warnings[i]);
  }
  if (result.warnings.size() > 5) {
    spdlog::warn("[CreatureXmlReader] ... and {} more warnings",
                 result.warnings.size() - 5);
  }

  return result;
}

std::unique_ptr<Domain::CreatureType>
CreatureXmlReader::parseCreatureNode(const pugi::xml_node &node, bool isNpc,
                                     std::vector<std::string> &warnings) {

  pugi::xml_attribute nameAttr = node.attribute("name");
  if (!nameAttr) {
    warnings.push_back("Creature node missing 'name' attribute");
    return nullptr;
  }

  auto creature = std::make_unique<Domain::CreatureType>();
  creature->name = nameAttr.as_string();
  creature->is_npc = isNpc;

  // Parse appearance - looktype (outfit) or lookitem
  if (auto attr = node.attribute("looktype")) {
    creature->outfit.lookType = static_cast<uint16_t>(attr.as_uint());
  }
  if (auto attr = node.attribute("lookitem")) {
    creature->outfit.lookItem = static_cast<uint16_t>(attr.as_uint());
  }
  if (auto attr = node.attribute("lookmount")) {
    creature->outfit.lookMount = static_cast<uint16_t>(attr.as_uint());
  }
  if (auto attr = node.attribute("lookaddon")) {
    creature->outfit.lookAddons = static_cast<uint16_t>(attr.as_uint());
  }

  // Outfit colors
  if (auto attr = node.attribute("lookhead")) {
    creature->outfit.lookHead = static_cast<uint16_t>(attr.as_uint());
  }
  if (auto attr = node.attribute("lookbody")) {
    creature->outfit.lookBody = static_cast<uint16_t>(attr.as_uint());
  }
  if (auto attr = node.attribute("looklegs")) {
    creature->outfit.lookLegs = static_cast<uint16_t>(attr.as_uint());
  }
  if (auto attr = node.attribute("lookfeet")) {
    creature->outfit.lookFeet = static_cast<uint16_t>(attr.as_uint());
  }

  // Mount colors (optional, for full RME compatibility)
  if (auto attr = node.attribute("lookmounthead")) {
    creature->outfit.lookMountHead = static_cast<uint16_t>(attr.as_uint());
  }
  if (auto attr = node.attribute("lookmountbody")) {
    creature->outfit.lookMountBody = static_cast<uint16_t>(attr.as_uint());
  }
  if (auto attr = node.attribute("lookmountlegs")) {
    creature->outfit.lookMountLegs = static_cast<uint16_t>(attr.as_uint());
  }
  if (auto attr = node.attribute("lookmountfeet")) {
    creature->outfit.lookMountFeet = static_cast<uint16_t>(attr.as_uint());
  }

  return creature;
}

} // namespace MapEditor::IO
