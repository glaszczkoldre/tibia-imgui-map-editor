#include "Services/ItemDefinitionResolver.h"
#include "Domain/ItemType.h"
#include "Domain/Item.h"
#include "Domain/Tile.h"
#include "IO/Loader/SourceFragments.h"
#include "IO/Readers/DatReaderBase.h"
#include <iostream>
#include <vector>
#include <stdexcept>

using namespace MapEditor;
using namespace MapEditor::Domain;
using namespace MapEditor::Services;

namespace {
void require(bool condition, const std::string &msg) {
  if (!condition) {
    throw std::runtime_error(msg);
  }
}
} // namespace

void testOtbGroundDatNonGround() {
  std::vector<IO::ServerItemFragment> server_items;
  IO::ServerItemFragment otb;
  otb.server_id = 100;
  otb.client_id = 200;
  otb.group = ItemGroup::Ground;
  server_items.push_back(otb);

  IO::DatResult dat_result;
  IO::DatItemFragment dat;
  dat.id = 200;
  dat.is_ground = false; // non-ground in DAT
  dat_result.items.push_back(dat);

  auto resolved = ItemDefinitionResolver::resolve(ItemDataSource::OTB, server_items, dat_result);
  require(resolved.size() == 1, "Expected 1 resolved item");
  require(resolved[0].isGround(), "OTB ground + DAT non-ground should resolve as ground in DatOtb");
}

void testDatGroundOtbNonGround() {
  std::vector<IO::ServerItemFragment> server_items;
  IO::ServerItemFragment otb;
  otb.server_id = 100;
  otb.client_id = 200;
  otb.group = ItemGroup::None; // non-ground in OTB
  server_items.push_back(otb);

  IO::DatResult dat_result;
  IO::DatItemFragment dat;
  dat.id = 200;
  dat.is_ground = true; // ground in DAT
  dat_result.items.push_back(dat);

  auto resolved = ItemDefinitionResolver::resolve(ItemDataSource::OTB, server_items, dat_result);
  require(resolved.size() == 1, "Expected 1 resolved item");
  require(!resolved[0].isGround(), "DAT ground + OTB non-ground should resolve as non-ground in DatOtb due to server precedence");
}

void testDatGroundInDatOnly() {
  std::vector<IO::ServerItemFragment> server_items; // Empty for DatOnly

  IO::DatResult dat_result;
  IO::DatItemFragment dat;
  dat.id = 200;
  dat.is_ground = true; // ground in DAT
  dat_result.items.push_back(dat);

  auto resolved = ItemDefinitionResolver::resolve(ItemDataSource::DAT, server_items, dat_result);
  require(resolved.size() == 1, "Expected 1 resolved item");
  require(resolved[0].isGround(), "DAT ground should resolve as ground in DatOnly");
}

void testXmlExplicitGroupOverride() {
  std::vector<IO::ServerItemFragment> server_items;
  IO::ServerItemFragment otb;
  otb.server_id = 100;
  otb.client_id = 200;
  otb.group = ItemGroup::Ground;
  server_items.push_back(otb);

  IO::DatResult dat_result;
  IO::DatItemFragment dat;
  dat.id = 200;
  dat.is_ground = true;
  dat_result.items.push_back(dat);

  auto resolved = ItemDefinitionResolver::resolve(ItemDataSource::OTB, server_items, dat_result);
  require(resolved[0].isGround(), "Initial state check failed");

  IO::XmlItemFragment xml;
  xml.server_id = 100;
  xml.group = ItemGroup::Container; // Override to container
  
  ItemDefinitionResolver::mergeXmlOverride(resolved[0], xml);
  ItemDefinitionResolver::normalizeResolvedItemTypes(resolved);

  require(resolved[0].isContainer(), "XML explicit group override should change final group");
  require(!resolved[0].isGround(), "XML override should reset ground type");
}

void testDatMovementFlagsDoNotOverrideOtb() {
  std::vector<IO::ServerItemFragment> server_items;
  IO::ServerItemFragment otb;
  otb.server_id = 100;
  otb.client_id = 200;
  otb.flags = ItemFlag::Moveable; // moveable in OTB
  server_items.push_back(otb);

  IO::DatResult dat_result;
  IO::DatItemFragment dat;
  dat.id = 200;
  dat.is_unmoveable = true; // unmoveable in DAT
  dat_result.items.push_back(dat);

  auto resolved = ItemDefinitionResolver::resolve(ItemDataSource::OTB, server_items, dat_result);
  require(resolved.size() == 1, "Expected 1 resolved item");
  require(resolved[0].isMoveable(), "DAT movement flags should not override OTB moveability in DatOtb");
}

void testDatAttributesFillVisuals() {
  std::vector<IO::ServerItemFragment> server_items;
  IO::ServerItemFragment otb;
  otb.server_id = 100;
  otb.client_id = 200;
  server_items.push_back(otb);

  IO::DatResult dat_result;
  IO::DatItemFragment dat;
  dat.id = 200;
  dat.has_light = true;
  dat.light_level = 7;
  dat.light_color = 120;
  dat.has_elevation = true;
  dat.elevation = 4;
  dat.width = 2;
  dat.height = 2;
  dat_result.items.push_back(dat);

  auto resolved = ItemDefinitionResolver::resolve(ItemDataSource::OTB, server_items, dat_result);
  require(resolved.size() == 1, "Expected 1 resolved item");
  require(resolved[0].light_level == 7, "Light level should be resolved from DAT");
  require(resolved[0].light_color == 120, "Light color should be resolved from DAT");
  require(resolved[0].hasElevation(), "Elevation flag should be set");
  require(resolved[0].elevation == 4, "Elevation value should be resolved from DAT");
  require(resolved[0].width == 2 && resolved[0].height == 2, "Visual dimensions should be filled from DAT");
}

void testXmlOverridesApplyOnce() {
  std::vector<IO::ServerItemFragment> server_items;
  IO::ServerItemFragment otb;
  otb.server_id = 100;
  otb.client_id = 200;
  server_items.push_back(otb);

  IO::DatResult dat_result;
  IO::DatItemFragment dat;
  dat.id = 200;
  dat_result.items.push_back(dat);

  auto resolved = ItemDefinitionResolver::resolve(ItemDataSource::OTB, server_items, dat_result);

  IO::XmlItemFragment xml;
  xml.server_id = 100;
  xml.name = "XML Override Name";
  xml.weapon_type = WeaponType::Sword;
  xml.slot_position = SlotPosition::Armor;
  xml.rotateTo = 3000;
  xml.charges = 5;

  ItemDefinitionResolver::mergeXmlOverride(resolved[0], xml);
  ItemDefinitionResolver::normalizeResolvedItemTypes(resolved);

  require(resolved[0].name == "XML Override Name", "XML name override failed");
  require(resolved[0].weapon_type == WeaponType::Sword, "XML weapon type override failed");
  require(resolved[0].slot_position == SlotPosition::Armor, "XML slot position override failed");
  require(resolved[0].rotateTo == 3000, "XML rotateTo override failed");
  require(resolved[0].isRotatable(), "XML rotation target should make item rotatable");
  require(resolved[0].charges == 5, "XML charges override failed");
}

void testServerMetadataSurvivesResolver() {
  std::vector<IO::ServerItemFragment> server_items;
  IO::ServerItemFragment otb;
  otb.server_id = 120;
  otb.client_id = 220;
  otb.volume = 8;
  otb.weight = 12.5f;
  otb.attack = 13;
  otb.defense = 14;
  otb.armor = 15;
  otb.charges = 16;
  otb.rotateTo = 221;
  server_items.push_back(otb);

  IO::DatResult dat_result;
  IO::DatItemFragment dat;
  dat.id = 220;
  dat_result.items.push_back(dat);

  auto resolved =
      ItemDefinitionResolver::resolve(ItemDataSource::OTB, server_items, dat_result);
  require(resolved.size() == 1, "Expected 1 resolved item");
  require(resolved[0].volume == 8, "Server volume should survive resolver");
  require(resolved[0].weight == 12.5f, "Server weight should survive resolver");
  require(resolved[0].attack == 13, "Server attack should survive resolver");
  require(resolved[0].defense == 14, "Server defense should survive resolver");
  require(resolved[0].armor == 15, "Server armor should survive resolver");
  require(resolved[0].charges == 16, "Server charges should survive resolver");
  require(resolved[0].rotateTo == 221, "Server rotate target should survive resolver");
}

void testXmlFlagClearOverrides() {
  std::vector<IO::ServerItemFragment> server_items;
  IO::ServerItemFragment otb;
  otb.server_id = 130;
  otb.client_id = 230;
  otb.flags = ItemFlag::Unpassable | ItemFlag::Pickupable |
              ItemFlag::AlwaysOnBottom | ItemFlag::CanReadText |
              ItemFlag::CanWriteText | ItemFlag::Rotatable |
              ItemFlag::ClientCharges;
  otb.maxTextLen = 64;
  otb.rotateTo = 231;
  otb.charges = 3;
  server_items.push_back(otb);

  IO::DatResult dat_result;
  IO::DatItemFragment dat;
  dat.id = 230;
  dat_result.items.push_back(dat);

  auto resolved =
      ItemDefinitionResolver::resolve(ItemDataSource::OTB, server_items, dat_result);

  IO::XmlItemFragment xml;
  xml.server_id = 130;
  xml.is_unpassable = false;
  xml.is_pickupable = false;
  xml.is_alwaysontop = false;
  xml.can_read_text = false;
  xml.can_write_text = false;
  xml.maxTextLen = 0;
  xml.rotateTo = 0;
  xml.charges = 0;

  ItemDefinitionResolver::mergeXmlOverride(resolved[0], xml);
  ItemDefinitionResolver::normalizeResolvedItemTypes(resolved);

  require(!resolved[0].hasFlag(ItemFlag::Unpassable), "XML should clear unpassable");
  require(!resolved[0].hasFlag(ItemFlag::Pickupable), "XML should clear pickupable");
  require(!resolved[0].hasFlag(ItemFlag::AlwaysOnBottom), "XML should clear always-on-bottom");
  require(!resolved[0].canReadText(), "XML should clear readable");
  require(!resolved[0].canWriteText(), "XML should clear writeable");
  require(!resolved[0].isRotatable(), "XML rotateTo=0 should clear rotatable");
  require(!resolved[0].hasFlag(ItemFlag::ClientCharges), "XML charges=0 should clear charges flag");
}

void testXmlClientIdRemapUsesNewDatVisuals() {
  IO::ServerItemFragment server;
  server.server_id = 140;
  server.client_id = 240;

  IO::DatItemFragment oldDat;
  oldDat.id = 240;
  oldDat.is_horizontal = true;
  oldDat.sprite_ids = {1001};

  IO::DatItemFragment newDat;
  newDat.id = 241;
  newDat.width = 2;
  newDat.height = 2;
  newDat.is_vertical = true;
  newDat.sprite_ids = {2001, 2002, 2003, 2004};

  IO::DatResult initialDat;
  initialDat.items.push_back(oldDat);

  auto initial = ItemDefinitionResolver::resolve(
      ItemDataSource::OTB, std::vector<IO::ServerItemFragment>{server}, initialDat);
  require(initial[0].hookEast(), "Initial DAT hook east setup failed");

  IO::XmlItemFragment xml;
  xml.server_id = 140;
  xml.client_id = 241;

  auto remappedServer = server;
  remappedServer.client_id = xml.client_id.value();
  IO::DatResult remappedDat;
  remappedDat.items.push_back(newDat);
  auto remapped = ItemDefinitionResolver::resolve(
      ItemDataSource::OTB, std::vector<IO::ServerItemFragment>{remappedServer},
      remappedDat);
  ItemDefinitionResolver::mergeXmlOverride(remapped[0], xml);
  ItemDefinitionResolver::normalizeResolvedItemTypes(remapped);

  require(remapped[0].client_id == 241, "XML client id override failed");
  require(remapped[0].width == 2 && remapped[0].height == 2,
          "XML client id remap should use new DAT dimensions");
  require(remapped[0].sprite_ids.size() == 4 && remapped[0].sprite_ids[0] == 2001,
          "XML client id remap should use new DAT sprites");
  require(!remapped[0].hookEast(), "Old DAT hook flag should not survive client id remap");
  require(remapped[0].hookSouth(), "New DAT hook flag should apply after client id remap");
}

void testLockedDoorAnnotationBacksHelper() {
  std::vector<IO::ServerItemFragment> server_items;
  IO::ServerItemFragment door;
  door.server_id = 150;
  door.client_id = 250;
  door.group = ItemGroup::Door;
  server_items.push_back(door);

  IO::DatResult dat_result;
  IO::DatItemFragment dat;
  dat.id = 250;
  dat_result.items.push_back(dat);

  auto resolved =
      ItemDefinitionResolver::resolve(ItemDataSource::OTB, server_items, dat_result);
  require(!resolved[0].isLocked(), "Door should not be locked before XML annotation");

  IO::XmlItemFragment xml;
  xml.server_id = 150;
  xml.editor_suffix = " (Locked)";

  ItemDefinitionResolver::mergeXmlOverride(resolved[0], xml);
  ItemDefinitionResolver::normalizeResolvedItemTypes(resolved);
  require(resolved[0].isLocked(), "Locked door XML annotation should back isLocked");
}

void testHookFlagsFromOtb() {
  std::vector<IO::ServerItemFragment> server_items;
  IO::ServerItemFragment otb_wall;
  otb_wall.server_id = 500;
  otb_wall.client_id = 600;
  otb_wall.flags = ItemFlag::HookSouth | ItemFlag::HookEast;
  server_items.push_back(otb_wall);

  IO::DatResult dat_result;
  IO::DatItemFragment dat;
  dat.id = 600;
  dat_result.items.push_back(dat);

  auto resolved = ItemDefinitionResolver::resolve(ItemDataSource::OTB, server_items, dat_result);
  require(resolved.size() == 1, "Expected 1 resolved item");
  require(resolved[0].hookSouth(), "HookSouth flag from OTB should survive resolver");
  require(resolved[0].hookEast(), "HookEast flag from OTB should survive resolver");
}

void testHookFlagsFromDat() {
  std::vector<IO::ServerItemFragment> server_items;
  IO::ServerItemFragment otb;
  otb.server_id = 500;
  otb.client_id = 600;
  server_items.push_back(otb);

  IO::DatResult dat_result;
  IO::DatItemFragment dat;
  dat.id = 600;
  dat.is_vertical = true;
  dat.is_horizontal = true;
  dat_result.items.push_back(dat);

  auto resolved = ItemDefinitionResolver::resolve(ItemDataSource::OTB, server_items, dat_result);
  require(resolved.size() == 1, "Expected 1 resolved item");
  require(resolved[0].hookSouth(), "HookSouth flag from DAT (is_vertical) should be set");
  require(resolved[0].hookEast(), "HookEast flag from DAT (is_horizontal) should be set");
}

void testHangableFlagFromDat() {
  std::vector<IO::ServerItemFragment> server_items;
  IO::ServerItemFragment otb;
  otb.server_id = 700;
  otb.client_id = 800;
  server_items.push_back(otb);

  IO::DatResult dat_result;
  IO::DatItemFragment dat;
  dat.id = 800;
  dat.is_hangable = true;
  dat_result.items.push_back(dat);

  auto resolved = ItemDefinitionResolver::resolve(ItemDataSource::OTB, server_items, dat_result);
  require(resolved.size() == 1, "Expected 1 resolved item");
  require(resolved[0].isHangable(), "IsHangable flag from DAT should be set");
}

void testWriteableGroupSurvivesResolver() {
  std::vector<IO::ServerItemFragment> server_items;
  IO::ServerItemFragment otb;
  otb.server_id = 900;
  otb.client_id = 901;
  otb.group = ItemGroup::Writeable;
  server_items.push_back(otb);

  IO::DatResult dat_result;
  IO::DatItemFragment dat;
  dat.id = 901;
  dat_result.items.push_back(dat);

  auto resolved = ItemDefinitionResolver::resolve(ItemDataSource::OTB, server_items, dat_result);
  require(resolved.size() == 1, "Expected 1 resolved item");
  require(resolved[0].isWriteable(), "OTB writeable group should remain writeable");
}

void testDatWritableSetsTextFlags() {
  std::vector<IO::ServerItemFragment> server_items;
  IO::ServerItemFragment otb;
  otb.server_id = 910;
  otb.client_id = 911;
  server_items.push_back(otb);

  IO::DatResult dat_result;
  IO::DatItemFragment dat;
  dat.id = 911;
  dat.is_writable = true;
  dat.max_text_length = 128;
  dat_result.items.push_back(dat);

  auto resolved = ItemDefinitionResolver::resolve(ItemDataSource::OTB, server_items, dat_result);
  require(resolved.size() == 1, "Expected 1 resolved item");
  require(resolved[0].canReadText(), "DAT writable item should be readable");
  require(resolved[0].canWriteText(), "DAT writable item should be writeable");
  require(resolved[0].isWriteable(), "DAT writable item should pass isWriteable");
  require(resolved[0].maxTextLen == 128, "DAT writable max text length should be preserved");
}

void testBorderAndWallHelpersUseResolvedFields() {
  ItemType borderType;
  borderType.flags = ItemFlag::AlwaysOnBottom;
  borderType.always_on_top_order = 1;
  require(borderType.isBorder(), "Top-order 1 always-on-bottom item should be treated as border");

  ItemType wallType;
  wallType.flags = ItemFlag::Unpassable | ItemFlag::BlockMissiles;
  wallType.always_on_top_order = 2;
  require(wallType.isWall(), "Blocking top-order item should be treated as wall");

  ItemType moveableBlockingType;
  moveableBlockingType.flags =
      ItemFlag::Unpassable | ItemFlag::BlockMissiles | ItemFlag::Moveable;
  moveableBlockingType.always_on_top_order = 2;
  require(!moveableBlockingType.isWall(), "Moveable blocking item should not be treated as wall");
}

void testTileHookDetection() {
  ItemType wallType;
  wallType.server_id = 500;
  wallType.client_id = 600;
  wallType.flags = ItemFlag::HookSouth | ItemFlag::HookEast;

  require(wallType.hookSouth(), "wallType.hookSouth() should return true");
  require(wallType.hookEast(), "wallType.hookEast() should return true");

  Domain::Tile tile(Domain::Position(10, 10, 7));
  auto wallItem = std::make_unique<Domain::Item>(500);
  wallItem->setType(&wallType);
  tile.addItem(std::move(wallItem));

  require(tile.hasHookSouth(), "Tile should detect hook south from wall item");
  require(tile.hasHookEast(), "Tile should detect hook east from wall item");
}

int main() {
  try {
    std::cout << "Running ItemDatabaseResolver tests...\n";
    testOtbGroundDatNonGround();
    testDatGroundOtbNonGround();
    testDatGroundInDatOnly();
    testXmlExplicitGroupOverride();
    testDatMovementFlagsDoNotOverrideOtb();
    testDatAttributesFillVisuals();
    testXmlOverridesApplyOnce();
    testServerMetadataSurvivesResolver();
    testXmlFlagClearOverrides();
    testXmlClientIdRemapUsesNewDatVisuals();
    testLockedDoorAnnotationBacksHelper();
    testHookFlagsFromOtb();
    testHookFlagsFromDat();
    testHangableFlagFromDat();
    testWriteableGroupSurvivesResolver();
    testDatWritableSetsTextFlags();
    testBorderAndWallHelpersUseResolvedFields();
    testTileHookDetection();
    std::cout << "All ItemDatabaseResolver tests passed!\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "Test failed with error: " << ex.what() << "\n";
    return 1;
  }
}
