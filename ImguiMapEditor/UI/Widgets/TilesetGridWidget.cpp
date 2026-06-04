#include "TilesetGridWidget.h"

#include "UI/Core/Theme.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>
#include <limits>
#include <string_view>

#include "ext/fontawesome6/IconsFontAwesome6.h"
#include <imgui.h>
#include <spdlog/spdlog.h>

#include "../../Brushes/BrushController.h"
#include "../../Brushes/Types/CreatureBrush.h"
#include "../../Brushes/Types/RawBrush.h"
#include "../../Domain/ItemType.h"
#include "../../Rendering/Core/Texture.h"
#include "../../Services/AppSettings.h"
#include "../../Services/ClientDataService.h"
#include "../../Services/SpriteManager.h"
#include "../Utils/PreviewUtils.hpp"
#include "../Utils/UIUtils.hpp"

namespace MapEditor::UI {

using namespace Domain::Tileset;

TilesetGridWidget::TilesetGridWidget() = default;

TilesetGridWidget::~TilesetGridWidget() = default;

void TilesetGridWidget::initialize(
    Services::ClientDataService *clientData,
    Services::SpriteManager *spriteManager,
    Brushes::BrushController *brushController,
    Domain::Tileset::TilesetRegistry &tilesetRegistry,
    Services::AppSettings *appSettings) {
  clientData_ = clientData;
  spriteManager_ = spriteManager;
  brushController_ = brushController;
  tilesetRegistry_ = &tilesetRegistry;
  appSettings_ = appSettings;
}

float TilesetGridWidget::getIconSize() const {
  if (appSettings_) {
    return appSettings_->paletteIconSize;
  }
  return iconSizeFallback_;
}

void TilesetGridWidget::setTileset(const std::string &tilesetName) {
  if (tilesetName_ != tilesetName) {
    tilesetName_ = tilesetName;
    filterDirty_ = true;
  }
}

Rendering::Texture *TilesetGridWidget::getBrushTexture(const Brushes::IBrush *brush) const {
  if (!clientData_ || !spriteManager_ || !brush) {
    return nullptr;
  }

  // Try RawBrush first
  if (auto *rawBrush = dynamic_cast<const Brushes::RawBrush *>(brush)) {
    const auto *itemType = clientData_->getItemTypeByServerId(
        static_cast<uint16_t>(rawBrush->getItemId()));
    if (auto *tex = Utils::GetItemPreview(*spriteManager_, itemType)) {
      return tex;
    }
  }
  // Try CreatureBrush
  else if (auto *creatureBrush =
               dynamic_cast<const Brushes::CreatureBrush *>(brush)) {
    const auto &outfit = creatureBrush->getOutfit();
    auto preview =
        Utils::GetCreaturePreview(*clientData_, *spriteManager_, outfit);
    if (preview.texture) {
      return preview.texture;
    }
  }

  return nullptr;
}

void TilesetGridWidget::renderBrushCard(ImVec2 cursorPos, ImVec2 size,
                                         Rendering::Texture *tex,
                                         bool isSelected, bool isHovered,
                                         bool isPulsing, float pulseElapsed) {
  ImDrawList *dl = ImGui::GetWindowDrawList();
  constexpr float ROUNDING = 4.0f;
  constexpr float PADDING = 2.0f;
  ImVec2 rectMax(cursorPos.x + size.x, cursorPos.y + size.y);

  ImU32 bgCol = ImGui::GetColorU32(ImGuiCol_FrameBg);
  if (isSelected) {
    bgCol = ImGui::GetColorU32(ImGuiCol_Header);
  } else if (isHovered) {
    bgCol = ImGui::GetColorU32(ImGuiCol_HeaderHovered);
  }
  dl->AddRectFilled(cursorPos, rectMax, bgCol, ROUNDING);

  if (tex) {
    ImVec2 imgMin(cursorPos.x + PADDING, cursorPos.y + PADDING);
    ImVec2 imgMax(rectMax.x - PADDING, rectMax.y - PADDING);
    dl->AddImageRounded((void *)(intptr_t)tex->id(), imgMin, imgMax,
                        ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, ROUNDING);
  }

  if (isSelected) {
    if (isPulsing && pulseElapsed < PULSE_DURATION) {
      float pulse = 0.5f + 0.5f * std::sin(pulseElapsed * 8.0f);
      ImU32 pulseCol = IM_COL32(static_cast<int>(50 * (1 - pulse)),
                                 static_cast<int>(220 * pulse + 35),
                                 static_cast<int>(80 * pulse), 255);
      float thickness = 2.0f + pulse * 2.0f;
      dl->AddRect(cursorPos, rectMax, pulseCol, ROUNDING, 0, thickness);
    } else {
      dl->AddRect(cursorPos, rectMax, IM_COL32(100, 180, 255, 255), ROUNDING,
                  0, 2.0f);
    }
  }
}

void TilesetGridWidget::render() {
  if (tilesetName_.empty()) {
    ImGui::TextDisabled(ICON_FA_BOX_OPEN " No tileset selected");
    return;
  }

  renderFilterInput();
  ImGui::Separator();
  renderBrushGrid();
}

void TilesetGridWidget::renderControlsOnly(bool /*vertical*/) {
  // Note: vertical parameter currently unused, kept for potential future layout
  // differences
  ImGui::SetNextItemWidth(-1);
  if (ImGui::InputTextWithHint("##Filter", ICON_FA_FILTER " Filter...",
                               filterBuffer_, sizeof(filterBuffer_))) {
    filterDirty_ = true;
  }
  Utils::SetTooltipOnHover("Filter brushes by name");

  if (!std::string_view(filterBuffer_).empty()) {
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_XMARK "##ClearFilter")) {
      filterBuffer_[0] = '\0';
      filterDirty_ = true;
    }
    Utils::SetTooltipOnHover("Clear filter");
  }
}

void TilesetGridWidget::renderGridOnly() {
  if (tilesetName_.empty()) {
    ImGui::TextDisabled(ICON_FA_BOX_OPEN " No tileset selected");
    return;
  }

  renderBrushGrid();
}

void TilesetGridWidget::renderFilterInput() {
  float availableWidth = ImGui::GetContentRegionAvail().x;
  ImGui::SetNextItemWidth(availableWidth - 130.0f);
  if (ImGui::InputTextWithHint("##Filter", ICON_FA_FILTER " Filter...",
                               filterBuffer_, sizeof(filterBuffer_))) {
    filterDirty_ = true;
  }
  Utils::SetTooltipOnHover("Filter brushes by name");

  if (!std::string_view(filterBuffer_).empty()) {
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_XMARK "##ClearFilter")) {
      filterBuffer_[0] = '\0';
      filterDirty_ = true;
    }
    Utils::SetTooltipOnHover("Clear filter");
  }
}

void TilesetGridWidget::applyFilter() {
  if (!tilesetRegistry_) {
    filteredEntries_.clear();
    return;
  }
  auto *tileset = tilesetRegistry_->getTileset(tilesetName_);
  if (!tileset) {
    filteredEntries_.clear();
    return;
  }

  const auto &entries = tileset->getEntries();

  std::string lowerFilter = filterBuffer_;
  std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  bool useCrossSearch = !lowerFilter.empty() && !allBrushes_.empty();

  if (useCrossSearch) {
    crossFilteredBrushes_.clear();
    crossFilteredBrushes_.reserve(allBrushes_.size());
    filteredEntries_.clear();

    std::copy_if(allBrushes_.begin(), allBrushes_.end(),
                 std::back_inserter(crossFilteredBrushes_),
                 [&](const BrushWithSource &entry) {
                   const auto &brushName = entry.brush->getName();
                   return std::search(brushName.begin(), brushName.end(),
                                      lowerFilter.begin(), lowerFilter.end(),
                                      [](unsigned char c1, unsigned char c2) {
                                        return std::tolower(c1) == c2;
                                      }) != brushName.end();
                 });
  } else {
    crossFilteredBrushes_.clear();
    filteredEntries_.clear();
    filteredEntries_.reserve(entries.size());

    for (size_t i = 0; i < entries.size(); ++i) {
      const auto &entry = entries[i];

      if (isSeparator(entry)) {
        if (lowerFilter.empty()) {
          filteredEntries_.push_back({i, entry});
        }
      } else if (isBrush(entry)) {
        const auto *brush = getBrush(entry);
        if (brush) {
          if (lowerFilter.empty()) {
            filteredEntries_.push_back({i, entry});
          } else {
            const auto &brushName = brush->getName();
            bool matches = std::search(brushName.begin(), brushName.end(),
                                       lowerFilter.begin(), lowerFilter.end(),
                                       [](unsigned char c1, unsigned char c2) {
                                         return std::tolower(c1) == c2;
                                       }) != brushName.end();
            if (matches) {
              filteredEntries_.push_back({i, entry});
            }
          }
        }
      }
    }
  }
}

void TilesetGridWidget::renderBrushGrid() {
  if (!tilesetRegistry_) {
    ImGui::TextDisabled(ICON_FA_TRIANGLE_EXCLAMATION
                        " Registry not initialized");
    return;
  }
  auto *tileset = tilesetRegistry_->getTileset(tilesetName_);
  if (!tileset) {
    ImGui::TextDisabled(ICON_FA_TRIANGLE_EXCLAMATION " Tileset not found");
    return;
  }

  if (tileset->getEntries().empty()) {
    ImGui::TextDisabled(ICON_FA_BOX_OPEN " No brushes in this tileset");
    return;
  }

  ImGui::BeginChild("BrushGrid", ImVec2(0, 0), true);

  if (filterDirty_) {
    applyFilter();
    filterDirty_ = false;
  }

  bool showingCrossResults = !crossFilteredBrushes_.empty();

  if (filteredEntries_.empty() && crossFilteredBrushes_.empty() &&
      !std::string_view(filterBuffer_).empty()) {
    ImGui::TextDisabled(ICON_FA_FILTER_CIRCLE_XMARK " No brushes match filter");
    ImGui::EndChild();
    return;
  }

  float availableWidth = ImGui::GetContentRegionAvail().x;
  float itemSpacingX = ImGui::GetStyle().ItemSpacing.x;
  float actualItemWidth = getIconSize() + itemSpacingX;
  int columns =
      std::max(1, static_cast<int>(std::floor((availableWidth + itemSpacingX) /
                                              actualItemWidth)));

  // Process pending brush selection (find by name and select)
  if (!pendingSelectBrushName_.empty()) {
    selectedIndices_.clear();
    bool found = false;

    // Search in filtered entries
    for (size_t i = 0; i < filteredEntries_.size() && !found; ++i) {
      if (isBrush(filteredEntries_[i].entry)) {
        const auto *brush = getBrush(filteredEntries_[i].entry);
        if (brush && brush->getName() == pendingSelectBrushName_) {
          selectedIndices_.insert(static_cast<int>(i));
          selectedBrushName_ = brush->getName();
          found = true;
        }
      }
    }
    pendingSelectBrushName_.clear();
  }

  if (showingCrossResults) {
    // Cross-tileset search results
    int col = 0;
    for (size_t i = 0; i < crossFilteredBrushes_.size(); ++i) {
      const auto &bws = crossFilteredBrushes_[i];
      const auto *brush = bws.brush;

      if (col > 0) {
        ImGui::SameLine();
      }

      ImGui::PushID(static_cast<int>(i));

      Rendering::Texture *tex = getBrushTexture(brush);
      ImVec2 tileSize(getIconSize(), getIconSize());
      ImVec2 cursorPos = ImGui::GetCursorScreenPos();
      ImGui::Dummy(tileSize);

      bool isCrossSelected = brush->getName() == selectedBrushName_;
      renderBrushCard(cursorPos, tileSize, tex, isCrossSelected,
                      ImGui::IsItemHovered());

      if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("%s", brush->getName().c_str());
        ImGui::TextDisabled("From: %s", bws.sourceTileset.c_str());
        ImGui::TextDisabled("Double-click to jump");
        ImGui::EndTooltip();
      }

      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
        if (onBrushDoubleClicked_) {
          onBrushDoubleClicked_(bws.sourceTileset, brush->getName());
        }
      }

      if (ImGui::IsItemClicked()) {
        selectedBrushName_ = brush->getName();
        if (brushController_) {
          brushController_->setBrush(const_cast<Brushes::IBrush *>(brush));
        }
        if (onBrushSelected_) {
          auto *rawBrush = dynamic_cast<const Brushes::RawBrush *>(brush);
          onBrushSelected_(rawBrush ? rawBrush->getItemId() : 0,
                           brush->getName());
        }
      }

      ImGui::PopID();

      col++;
      if (col >= columns) {
        col = 0;
      }
    }
  } else {
    // Single tileset entries
    int col = 0;
    for (size_t entryIdx = 0; entryIdx < filteredEntries_.size(); ++entryIdx) {
      const auto &fe = filteredEntries_[entryIdx];

      if (isSeparator(fe.entry)) {
        // Separator - render on new row
        if (col > 0) {
          col = 0;
        }

        const auto &sep = getSeparator(fe.entry);
        bool isCollapsed = collapsedSections_[fe.originalIndex];

        ImGui::PushID(static_cast<int>(fe.originalIndex + 10000));

        // Collapsible header style
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.2f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                              ImVec4(0.3f, 0.3f, 0.4f, 1.0f));

        std::string headerLabel = sep.name.empty() ? "---" : sep.name;
        if (ImGui::CollapsingHeader(headerLabel.c_str(),
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
          collapsedSections_[fe.originalIndex] = false;
        } else {
          collapsedSections_[fe.originalIndex] = true;
        }

        ImGui::PopStyleColor(2);
        ImGui::PopID();
        continue;
      }

      // Check if section is collapsed
      // Find previous separator
      bool inCollapsedSection = false;
      for (int j = static_cast<int>(entryIdx) - 1; j >= 0; --j) {
        if (isSeparator(filteredEntries_[j].entry)) {
          inCollapsedSection =
              collapsedSections_[filteredEntries_[j].originalIndex];
          break;
        }
      }

      if (inCollapsedSection) {
        continue;
      }

      const auto *brush = getBrush(fe.entry);
      if (!brush)
        continue;

      if (col > 0) {
        ImGui::SameLine();
      }

      ImGui::PushID(static_cast<int>(fe.originalIndex));

      ImVec2 tileSize(getIconSize(), getIconSize());
      ImVec2 cursorPos = ImGui::GetCursorScreenPos();

      Rendering::Texture *tex = getBrushTexture(brush);

      // Drag source
      ImGui::InvisibleButton("##tile", tileSize);
      bool isHovered = ImGui::IsItemHovered();
      bool isClicked = ImGui::IsItemClicked();
      bool isSelected = selectedIndices_.count(static_cast<int>(entryIdx)) > 0;

      // Scroll to this brush if it's the target
      if (!scrollToBrushName_.empty() &&
          brush->getName() == scrollToBrushName_) {
        ImGui::SetScrollHereY(0.5f);
        scrollToBrushName_.clear();
      }

      // Drag-drop source
      if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
        ImGui::SetDragDropPayload("TILESET_ENTRY", &fe.originalIndex,
                                  sizeof(size_t));
        ImGui::Text("Moving: %s", brush->getName().c_str());
        ImGui::EndDragDropSource();
      }

      // Drag-drop target
      if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload *payload =
                ImGui::AcceptDragDropPayload("TILESET_ENTRY")) {
          size_t sourceIdx = *static_cast<const size_t *>(payload->Data);
          if (sourceIdx != fe.originalIndex && tilesetRegistry_) {
            if (auto *ts = tilesetRegistry_->getTileset(tilesetName_)) {
              ts->moveEntry(sourceIdx, fe.originalIndex);
              filterDirty_ = true;
              if (onTilesetModified_) {
                onTilesetModified_(tilesetName_);
              }
            }
          }
        }
        ImGui::EndDragDropTarget();
      }

      // Render card with pulse animation support
      bool isPulsing = isSelected && !pulseBrushName_.empty() &&
                       brush->getName() == pulseBrushName_;
      float pulseElapsed = 0.0f;
      if (isPulsing) {
        float currentTime = ImGui::GetTime();
        if (pulseStartTime_ < 0)
          pulseStartTime_ = currentTime;
        pulseElapsed = currentTime - pulseStartTime_;
        if (pulseElapsed >= PULSE_DURATION) {
          pulseBrushName_.clear();
          pulseStartTime_ = -1.0f;
          isPulsing = false;
        }
      }
      renderBrushCard(cursorPos, tileSize, tex, isSelected, isHovered,
                      isPulsing, pulseElapsed);

      // Tooltip
      if (isHovered) {
        ImGui::BeginTooltip();
        ImGui::Text("%s", brush->getName().c_str());
        ImGui::EndTooltip();
      }

      // Click handling
      if (isClicked) {
        bool ctrlHeld = ImGui::GetIO().KeyCtrl;
        bool shiftHeld = ImGui::GetIO().KeyShift;

        if (ctrlHeld) {
          // Toggle selection
          if (isSelected) {
            selectedIndices_.erase(static_cast<int>(entryIdx));
          } else {
            selectedIndices_.insert(static_cast<int>(entryIdx));
          }
        } else if (shiftHeld && lastClickedIndex_ >= 0) {
          // Range select
          int start = std::min(lastClickedIndex_, static_cast<int>(entryIdx));
          int end = std::max(lastClickedIndex_, static_cast<int>(entryIdx));
          for (int k = start; k <= end; ++k) {
            selectedIndices_.insert(k);
          }
        } else {
          // Single select
          selectedIndices_.clear();
          selectedIndices_.insert(static_cast<int>(entryIdx));

          selectedBrushName_ = brush->getName();
          if (brushController_) {
            brushController_->setBrush(const_cast<Brushes::IBrush *>(brush));
          }
          if (onBrushSelected_) {
            auto *rawBrush = dynamic_cast<const Brushes::RawBrush *>(brush);
            onBrushSelected_(rawBrush ? rawBrush->getItemId() : 0,
                             brush->getName());
          }
        }
        lastClickedIndex_ = static_cast<int>(entryIdx);
      }

      ImGui::PopID();

      col++;
      if (col >= columns) {
        col = 0;
      }
    }
  }

  ImGui::EndChild();
}

} // namespace MapEditor::UI
