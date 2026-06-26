#include "Services/ItemDefinitionResolver.h"
#include "Domain/ItemType.h"
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
    std::cout << "All ItemDatabaseResolver tests passed!\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "Test failed with error: " << ex.what() << "\n";
    return 1;
  }
}
