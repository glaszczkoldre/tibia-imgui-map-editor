#include "ClientDataService.h"
#include "ItemDefinitionResolver.h"
#include "SpriteManager.h"
#include <algorithm>
#include <format>
#include <spdlog/spdlog.h>

namespace MapEditor {
namespace Services {

ClientDataResult
ClientDataService::load(const std::filesystem::path &client_path,
                        const std::filesystem::path &item_metadata_path,
                        const Domain::ClientVersion &client_version,
                        ::MapEditor::Domain::ItemDataSource data_source,
                        LoadProgressCallback progress) {
  ClientDataResult result;
  clear();

  uint32_t version_num = client_version.getVersion();

  if (progress)
    progress(0, "Loading item database...");

  // 1. Load item definitions (OTB or SRV format)
  std::vector<IO::ServerItemFragment> item_definitions;

  if (data_source == ::MapEditor::Domain::ItemDataSource::DAT) {
    spdlog::info("ClientDataService: Using DAT-only mode (Client IDs as Server IDs)");
  } else if (data_source == ::MapEditor::Domain::ItemDataSource::SRV) {
    // Load SRV format
    std::filesystem::path srv_path = item_metadata_path;

    IO::SrvResult srv_result = IO::SrvReader::read(srv_path);
    if (!srv_result.success) {
      result.error = "Failed to load SRV: " + srv_result.error;
      return result;
    }

    item_definitions = std::move(srv_result.items);

    // SRV format is from 7.x era
    result.otb_version.major_version = 0;
    result.otb_version.minor_version = 0;
    result.otb_version.build_number = 0;
    result.otb_version.valid = false;

    spdlog::info("SRV loaded: {} items (ancient 7.x format)",
                 item_definitions.size());
  } else {
    // Load OTB format (default)
    IO::OtbResult otb_result = IO::OtbReader::read(item_metadata_path);
    if (!otb_result.success) {
      result.error = "Failed to load OTB: " + otb_result.error;
      return result;
    }

    item_definitions = std::move(otb_result.items);
    result.otb_version = otb_result.version;

    spdlog::info("OTB loaded: {} items, version {}.{}.{}",
                 item_definitions.size(), otb_result.version.major_version,
                 otb_result.version.minor_version,
                 otb_result.version.build_number);
  }

  if (progress)
    progress(20, "Loading DAT...");

  // 2. Load DAT (client item appearances)
  std::filesystem::path dat_path = client_path / "Tibia.dat";
  auto dat_reader = IO::DatReaderFactory::create(version_num);

  if (!dat_reader) {
    result.error =
        "Unsupported client version: " + std::to_string(version_num);
    return result;
  }

  if (!std::filesystem::exists(dat_path)) {
    dat_path = client_path / "tibia.dat";
  }

  IO::DatResult dat_result = dat_reader->read(dat_path);
  if (!dat_result.success) {
    result.error = "Failed to read DAT file: " + dat_result.error;
    return result;
  }

  result.dat_signature = dat_result.signature;

  result.item_count = dat_result.items.size();
  result.outfit_count = dat_result.outfits.size();
  result.effect_count = dat_result.effects.size();
  result.missile_count = dat_result.missiles.size();

  spdlog::info("DAT loaded: {} items, {} outfits, {} effects, {} missiles",
               dat_result.items.size(), dat_result.outfits.size(),
               dat_result.effects.size(), dat_result.missiles.size());

  if (progress)
    progress(60, "Merging data...");

  // 3. Resolve data into one final item definition table.
  auto resolved_items = ItemDefinitionResolver::resolve(data_source, item_definitions,
                                                       dat_result);
  item_store_.setItems(std::move(resolved_items));
  max_server_id_ = item_store_.getMaxServerId();
  max_client_id_ = item_store_.getMaxClientId();

  // 4. Store outfit data for creature sprite lookup
  outfits_ = dat_result.outfits;
  outfit_index_.clear();
  for (size_t i = 0; i < outfits_.size(); ++i) {
    outfit_index_[outfits_[i].id] = i;
  }
  spdlog::info("Stored {} outfits for creature rendering", outfits_.size());

  if (progress)
    progress(80, "Initializing Sprites...");

  // 5. Initialize Sprite Reader (lazy)
  std::filesystem::path spr_path = client_path / "Tibia.spr";
  if (!std::filesystem::exists(spr_path)) {
    spr_path = client_path / "tibia.spr";
  }

  if (!spr_reader_) {
    spr_reader_ = std::make_shared<IO::SprReader>();
  }

  bool extended = client_version.isExtended();
  auto spr_result = spr_reader_->open(spr_path, 0, extended);

  if (spr_result.success) {
    result.spr_signature = spr_reader_->getSignature();
    result.sprite_count = spr_reader_->getSpriteCount();
    spdlog::info("SPR loaded: {} sprites", result.sprite_count);
  } else {
    spdlog::warn("Failed to open SPR file: {}", spr_path.string());
    result.error = "Failed to open SPR file: " + spr_result.error;
    return result;
  }

  result.success = true;
  loaded_ = true;
  client_version_ = version_num;

  if (progress)
    progress(100, "Done");

  return result;
}

bool ClientDataService::loadCreatureData(
    const std::filesystem::path &creatures_xml_path) {
  auto result = IO::CreatureXmlReader::read(creatures_xml_path);
  if (!result.success) {
    spdlog::error("Failed to load creatures.xml: {}", result.error);
    return false;
  }

  spdlog::info("Loaded {} creatures from XML", result.creatures.size());

  for (auto &creature : result.creatures) {
    std::string lower_name = creature->name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                   ::tolower);

    creature_map_[lower_name] = creature.get();
    creatures_.push_back(std::move(creature));
  }

  return true;
}

bool ClientDataService::loadItemData(
    const std::filesystem::path &items_xml_path) {
  if (!loaded_) {
    spdlog::warn("Cannot load items.xml before OTB/DAT data");
    return false;
  }

  auto xml_result = IO::ItemXmlReader::load(items_xml_path);
  if (!xml_result.success) {
    spdlog::warn("Failed to load items.xml: {}", xml_result.error);
    return false;
  }

  // Merge override fragments using the resolver
  size_t merged_count = 0;
  auto& items = item_store_.getItemTypes();
  
  std::unordered_map<uint16_t, size_t> server_id_index;
  server_id_index.reserve(items.size());
  for (size_t index = 0; index < items.size(); ++index) {
      if (items[index].server_id > 0) {
          server_id_index[items[index].server_id] = index;
      }
  }

  for (const auto &xml_fragment : xml_result.items) {
      auto it = server_id_index.find(xml_fragment.server_id);
      if (it != server_id_index.end()) {
          ItemDefinitionResolver::mergeXmlOverride(items[it->second], xml_fragment);
          merged_count++;
      }
  }

  // Reindex the store after modifications
  item_store_.rebuildIndexes();
  max_server_id_ = item_store_.getMaxServerId();
  max_client_id_ = item_store_.getMaxClientId();

  spdlog::info("Loaded {} items from XML, merged {} with existing types",
               xml_result.items_loaded, merged_count);
  ItemDefinitionResolver::normalizeResolvedItemTypes(items);
  return true;
}

const Domain::CreatureType *
ClientDataService::getCreatureType(const std::string &name) const {
  std::string lower_name = name;
  std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                 ::tolower);

  auto it = creature_map_.find(lower_name);
  if (it != creature_map_.end()) {
    return it->second;
  }
  return nullptr;
}

size_t ClientDataService::optimizeItemSprites(SpriteManager& sprite_manager, bool preload_sprites) {
  size_t cached_count = 0;
  size_t simple_items = 0;

  auto& items = item_store_.getItemTypes();
  spdlog::info("Caching sprite regions for {} item types...", items.size());

  for (auto &item_type : items) {
    if (item_type.width == 1 && item_type.height == 1 &&
        item_type.layers == 1 && item_type.frames == 1 &&
        !item_type.sprite_ids.empty()) {

      simple_items++;
      uint32_t sprite_id = item_type.sprite_ids[0];

      if (sprite_id == 0)
        continue;

      const Rendering::AtlasRegion *region = nullptr;

      if (preload_sprites) {
        region = sprite_manager.preloadSprite(sprite_id);
      } else {
        region = sprite_manager.getSpriteRegion(sprite_id);
      }

      if (region) {
        item_type.cached_sprite_region = region;
        cached_count++;
      }
    }
  }

  spdlog::info(
      "Sprite caching complete: {} of {} simple items cached ({:.1f}%)",
      cached_count, simple_items,
      simple_items > 0 ? (100.0 * cached_count / simple_items) : 0.0);

  return cached_count;
}

void ClientDataService::clear() {
  item_store_.clear();
  max_server_id_ = 0;
  max_client_id_ = 0;

  creatures_.clear();
  creature_map_.clear();

  outfits_.clear();
  outfit_index_.clear();

  loaded_ = false;
  client_version_ = 0;
}

const IO::ClientItem *
ClientDataService::getOutfitData(uint16_t lookType) const {
  auto it = outfit_index_.find(lookType);
  if (it != outfit_index_.end()) {
    return &outfits_[it->second];
  }
  return nullptr;
}

std::vector<uint32_t>
ClientDataService::getOutfitSpriteIds(uint16_t lookType) const {
  auto it = outfit_index_.find(lookType);
  if (it != outfit_index_.end()) {
    return outfits_[it->second].sprite_ids;
  }
  return {};
}

} // namespace Services
} // namespace MapEditor
