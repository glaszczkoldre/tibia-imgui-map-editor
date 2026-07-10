#include "DoodadXmlHelper.h"

#include "Brushes/BrushRegistry.h"
#include "Brushes/Types/DoodadBrush.h"
#include "IO/BrushXmlReader.h"
#include <spdlog/spdlog.h>
#include <charconv>
#include <algorithm>
#include <cctype>
#include <optional>

namespace MapEditor::IO {

namespace {

bool isValidInteger(std::string_view text) {
  while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
    text.remove_prefix(1);
  }
  while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
    text.remove_suffix(1);
  }
  if (text.empty()) {
    return false;
  }
  size_t start = 0;
  if (text[0] == '-' || text[0] == '+') {
    start = 1;
    if (text.size() == 1) {
      return false;
    }
  }
  for (size_t i = start; i < text.size(); ++i) {
    if (!std::isdigit(static_cast<unsigned char>(text[i]))) {
      return false;
    }
  }
  return true;
}

std::optional<int32_t> parseInt(std::string_view text) {
  int32_t value = 0;
  const auto *begin = text.data();
  const auto *end = begin + text.size();
  const auto [ptr, ec] = std::from_chars(begin, end, value);
  if (ec != std::errc{} || ptr != end) {
    return std::nullopt;
  }
  return value;
}

std::optional<float> parseThicknessValue(std::string_view thickness) {
  if (thickness.empty()) {
    return std::nullopt;
  }

  if (const auto slash = thickness.find('/'); slash != std::string_view::npos) {
    const auto numerator = parseInt(thickness.substr(0, slash));
    const auto denominator = parseInt(thickness.substr(slash + 1));
    if (!numerator || !denominator) {
      return std::nullopt;
    }

    const auto safeNumerator = std::max(0, *numerator);
    const auto safeDenominator = std::max(1, *denominator);
    return static_cast<float>(safeNumerator) /
           static_cast<float>(safeDenominator);
  }

  const auto value = parseInt(thickness);
  if (!value) {
    return std::nullopt;
  }

  if (*value <= 1) {
    return static_cast<float>(*value);
  }

  return static_cast<float>(*value) / 100.0f;
}

bool readBoolAttribute(const pugi::xml_node &node, const char *name,
                       bool defaultValue = false) {
  const auto attribute = node.attribute(name);
  return attribute ? attribute.as_bool(defaultValue) : defaultValue;
}

bool readBoolAttributeAlias(const pugi::xml_node &node, const char *primary,
                             const char *alias, bool defaultValue = false) {
  const auto primaryAttribute = node.attribute(primary);
  if (primaryAttribute) {
    return primaryAttribute.as_bool(defaultValue);
  }

  const auto aliasAttribute = node.attribute(alias);
  if (aliasAttribute) {
    return aliasAttribute.as_bool(defaultValue);
  }

  return defaultValue;
}

Brushes::BrushPreviewDescriptor parsePreviewDescriptor(const pugi::xml_node &node) {
  if (const auto serverLookId = node.attribute("server_lookid").as_uint(0);
      serverLookId != 0) {
    return Brushes::BrushPreviewDescriptor::serverItem(serverLookId);
  }

  if (const auto lookId = node.attribute("lookid").as_uint(0); lookId != 0) {
    return Brushes::BrushPreviewDescriptor::serverItem(lookId);
  }

  return {};
}

bool isCollectionSourceFile(const std::filesystem::path &sourceFile) {
  std::string filename = sourceFile.filename().string();
  std::transform(filename.begin(), filename.end(), filename.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return filename.find("collection") != std::string::npos;
}

void markBrushMetadata(Brushes::IBrush &brush, const std::filesystem::path &sourceFile) {
  brush.flagAsVisible();
  if (isCollectionSourceFile(sourceFile)) {
    brush.setCollection();
  }
}

uint16_t parseItemId(const pugi::xml_node &node) {
  return static_cast<uint16_t>(
      node.attribute("id").as_uint(node.attribute("item").as_uint(0)));
}

uint32_t parseChance(const pugi::xml_node &node) {
  int32_t chanceVal = node.attribute("chance").as_int(1);
  return static_cast<uint32_t>(std::max(0, chanceVal));
}

std::optional<int32_t> parseChanceAttribute(const pugi::xml_node &node) {
  auto attr = node.attribute("chance");
  if (!attr) {
    return std::nullopt; // Missing
  }
  std::string_view val = attr.value();
  if (!isValidInteger(val)) {
    return std::nullopt; // Malformed
  }
  int32_t chanceVal = attr.as_int(1);
  if (chanceVal < 0) {
    chanceVal = 0; // Clamp negative values to zero
  }
  return chanceVal;
}

bool parseCompositeTile(const pugi::xml_node &tileNode, Brushes::CompositeItem::TileOffset &offset) {
  auto xAttr = tileNode.attribute("x");
  auto yAttr = tileNode.attribute("y");
  auto zAttr = tileNode.attribute("z");

  if (!xAttr || !yAttr) {
    return false; // Require x and y
  }
  if (!isValidInteger(xAttr.value()) || !isValidInteger(yAttr.value())) {
    return false; // Malformed x or y
  }
  if (zAttr && !isValidInteger(zAttr.value())) {
    return false; // Malformed z
  }

  int32_t dx = xAttr.as_int();
  int32_t dy = yAttr.as_int();
  int32_t dz = zAttr ? zAttr.as_int() : 0;

  if (dx < -32 || dx > 32 || dy < -32 || dy > 32 || dz < -15 || dz > 15) {
    return false; // Out of range
  }

  offset.dx = dx;
  offset.dy = dy;
  offset.dz = dz;
  return true;
}

Brushes::CompositeItem parseCompositeNode(const pugi::xml_node &compositeNode, const std::string &brushName) {
  Brushes::CompositeItem composite;
  for (const auto tileNode : compositeNode.children("tile")) {
    Brushes::CompositeItem::TileOffset offset;
    if (!parseCompositeTile(tileNode, offset)) {
      spdlog::warn("[DoodadXmlHelper] skipping malformed or out-of-range tile in composite under '{}'", brushName);
      continue;
    }

    for (const auto itemNode : tileNode.children("item")) {
      const auto itemId = parseItemId(itemNode);
      if (itemId == 0) {
        continue;
      }
      offset.items.push_back({
          .itemId = itemId,
          .chance = parseChance(itemNode),
          .subtype = itemNode.attribute("subtype").as_uint(0)
      });
    }

    if (!offset.items.empty()) {
      composite.tiles.push_back(std::move(offset));
    }
  }
  return composite;
}

} // namespace

void parseDoodadBrush(const pugi::xml_node &node,
                      const std::string &name,
                      uint32_t lookId,
                      const std::filesystem::path &sourceFile,
                      Brushes::BrushRegistry &brushRegistry) {
  // Default draggable to false
  auto brush = std::make_unique<Brushes::DoodadBrush>(
      name, lookId, brushRegistry,
      readBoolAttribute(node, "draggable", false));

  brush->setPreviewDescriptor(parsePreviewDescriptor(node));
  markBrushMetadata(*brush, sourceFile);
  brush->setRedoBorders(
      readBoolAttributeAlias(node, "redo_borders", "reborder", false));
  brush->setOneSize(readBoolAttribute(node, "one_size", false));
  brush->setRemoveOptionalBorder(
      readBoolAttribute(node, "remove_optional_border", false));
  brush->setOnBlocking(readBoolAttribute(node, "on_blocking", false));
  brush->setOnDuplicate(readBoolAttribute(node, "on_duplicate", false));

  if (brush->removesOptionalBorder() && !brush->needsBorderUpdate()) {
    spdlog::warn(
        "[DoodadXmlHelper] remove_optional_border on '{}' has no effect without redo_borders/reborder",
        name);
  }

  if (const auto thickness = parseThicknessValue(node.attribute("thickness").as_string());
      thickness.has_value()) {
    brush->setThickness(*thickness);
  }

  auto appendSingles = [&](const pugi::xml_node &parent,
                           Brushes::DoodadAlternative &alternative) {
    for (const auto itemNode : parent.children("item")) {
      const auto chanceOpt = parseChanceAttribute(itemNode);
      if (!chanceOpt) {
        spdlog::warn("[DoodadXmlHelper] missing or malformed chance attribute on item under '{}'", name);
        continue;
      }
      const auto itemId = parseItemId(itemNode);
      if (itemId != 0) {
        alternative.addSingleItem({
            .itemId = itemId,
            .chance = static_cast<uint32_t>(*chanceOpt),
            .subtype = itemNode.attribute("subtype").as_uint(0)
        });
      }
    }
  };

  auto appendComposites = [&](const pugi::xml_node &parent,
                              Brushes::DoodadAlternative &alternative) {
    for (const auto compositeNode : parent.children("composite")) {
      const auto chanceOpt = parseChanceAttribute(compositeNode);
      if (!chanceOpt) {
        spdlog::warn("[DoodadXmlHelper] missing or malformed chance attribute on composite under '{}'", name);
        continue;
      }
      auto composite = parseCompositeNode(compositeNode, name);
      if (!composite.tiles.empty()) {
        composite.chance = static_cast<uint32_t>(*chanceOpt);
        alternative.addComposite(std::move(composite));
      }
    }
  };

  bool addedAlternative = false;
  for (const auto altNode : node.children("alternate")) {
    Brushes::DoodadAlternative alternative;
    appendSingles(altNode, alternative);
    appendComposites(altNode, alternative);

    if (alternative.hasContent()) {
      brush->addAlternative(std::move(alternative));
      addedAlternative = true;
    }
  }

  if (!addedAlternative) {
    Brushes::DoodadAlternative alternative;
    appendSingles(node, alternative);
    appendComposites(node, alternative);

    if (alternative.hasContent()) {
      brush->addAlternative(std::move(alternative));
    }
  }

  brushRegistry.addBrush(std::move(brush));
}

} // namespace MapEditor::IO
