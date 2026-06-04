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
#include "../../Brushes/Types/RawBrush.h"
#include "../../Rendering/Core/Texture.h"
#include "../../Services/AppSettings.h"
#include "../../Services/ClientDataService.h"
#include "../../Services/SpriteManager.h"
#include "../../Domain/Tileset/Tileset.h"
#include "../../Domain/Tileset/TilesetEntry.h"
#include "../Utils/BrushPreviewRenderer.h"
#include "../Utils/BrushPreviewResolver.h"
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

void TilesetGridWidget::setTileset(Domain::Tileset::Tileset *tileset) {
  if (currentTileset_ != tileset) {
    currentTileset_ = tileset;
    tilesetName_ = tileset ? tileset->getName() : std::string{};
    filterDirty_ = true;
  }
}

Utils::ResolvedBrushPreview
TilesetGridWidget::getBrushPreview(const Brushes::IBrush *brush) const {
  return Utils::ResolveBrushPreview(brush, clientData_, spriteManager_);
}

std::pair<bool, float> TilesetGridWidget::computePulseState(const Brushes::IBrush *brush) {
  bool isPulsing = pulseBrush_ && brush == pulseBrush_;
  if (!isPulsing && !pulseBrushName_.empty()) {
    isPulsing = brush->getName() == pulseBrushName_;
  }
  if (!isPulsing) return {false, 0.0f};

  float currentTime = ImGui::GetTime();
  if (pulseStartTime_ < 0) pulseStartTime_ = currentTime;
  float elapsed = currentTime - pulseStartTime_;
  if (elapsed >= PULSE_DURATION) {
    pulseBrush_ = nullptr;
    pulseBrushName_.clear();
    pulseStartTime_ = -1.0f;
    return {false, 0.0f};
  }
  return {true, elapsed};
}

void TilesetGridWidget::syncActiveBrushSelection() {
  if (!brushController_) {
    return;
  }
  const auto *activeBrush = brushController_->getCurrentBrush();
  if (activeBrush == syncedActiveBrush_) {
    return;
  }
  syncedActiveBrush_ = activeBrush;
  if (!activeBrush) {
    selectedBrush_ = nullptr;
    selectedIndices_.clear();
    return;
  }

  selectedBrush_ = activeBrush;
  selectBrush(activeBrush, false, false);

  if (std::string_view filterValue{filterBuffer_}; !filterValue.empty()) {
    const auto matchesFilter =
        std::search(activeBrush->getName().begin(), activeBrush->getName().end(),
                    filterValue.begin(), filterValue.end(),
                    [](unsigned char lhs, unsigned char rhs) {
                      return std::tolower(lhs) == std::tolower(rhs);
                    }) != activeBrush->getName().end();
    if (!matchesFilter) {
      clearFilter();
    }
  }
}

void TilesetGridWidget::renderBrushCard(ImVec2 cursorPos, ImVec2 size,
                                         const Utils::ResolvedBrushPreview &preview,
                                         bool isSelected, bool isHovered,
                                         bool isPulsing, float pulseElapsed) {
  ImDrawList *dl = ImGui::GetWindowDrawList();
  constexpr float CARD_ROUNDING = 4.0f;
  constexpr float IMG_ROUNDING = 3.0f;
  constexpr float IMG_PADDING = 2.0f;
  ImVec2 rectMax(cursorPos.x + size.x, cursorPos.y + size.y);

  // Rounded card background using ImGui theme colors
  ImU32 bgCol = ImGui::GetColorU32(ImGuiCol_FrameBg);
  if (isSelected) {
    bgCol = ImGui::GetColorU32(ImGuiCol_Header);
  } else if (isHovered) {
    bgCol = ImGui::GetColorU32(ImGuiCol_HeaderHovered);
  }
  dl->AddRectFilled(cursorPos, rectMax, bgCol, CARD_ROUNDING);
  dl->AddRect(cursorPos, rectMax, ImGui::GetColorU32(ImGuiCol_Border), CARD_ROUNDING);

  // Rounded image inside card with padding
  if (preview.texture && preview.texture->isValid()) {
    ImVec2 imgMin(cursorPos.x + IMG_PADDING, cursorPos.y + IMG_PADDING);
    ImVec2 imgMax(rectMax.x - IMG_PADDING, rectMax.y - IMG_PADDING);
    dl->AddImageRounded(
        reinterpret_cast<void *>(static_cast<uintptr_t>(preview.texture->id())),
        imgMin, imgMax, ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, IMG_ROUNDING);
  } else if (!preview.fallbackLabel.empty()) {
    // Fallback text label — truncate to fit within card
    std::string label = preview.fallbackLabel;
    constexpr float MAX_TEXT_WIDTH = 28.0f; // 32 - 2*padding
    ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
    if (textSize.x > MAX_TEXT_WIDTH && !label.empty()) {
      // Estimate truncation length to minimize CalcTextSize calls
      float avgCharWidth = textSize.x / static_cast<float>(label.size());
      size_t estimated = static_cast<size_t>(MAX_TEXT_WIDTH / avgCharWidth);
      if (estimated > 3) {
        label = label.substr(0, estimated - 3);
      } else {
        label = label.substr(0, 1);
      }
      while (label.size() > 1 && ImGui::CalcTextSize((label + "...").c_str()).x > MAX_TEXT_WIDTH) {
        label.pop_back();
      }
      label += "...";
      textSize = ImGui::CalcTextSize(label.c_str());
    }
    ImVec2 textPos{cursorPos.x + (size.x - textSize.x) * 0.5f,
                   cursorPos.y + (size.y - textSize.y) * 0.5f};
    dl->AddText(textPos, IM_COL32(220, 220, 220, 255), label.c_str());
  }

  // Selection / pulse border
  if (isSelected) {
    if (isPulsing && pulseElapsed < PULSE_DURATION) {
      float pulse = 0.5f + 0.5f * std::sin(pulseElapsed * 8.0f);
      ImU32 pulseCol = IM_COL32(static_cast<int>(50 * (1 - pulse)),
                                 static_cast<int>(220 * pulse + 35),
                                 static_cast<int>(80 * pulse), 255);
      float thickness = 2.0f + pulse * 2.0f;
      dl->AddRect(cursorPos, rectMax, pulseCol, CARD_ROUNDING, 0, thickness);
    } else {
      dl->AddRect(cursorPos, rectMax, IM_COL32(100, 180, 255, 255), CARD_ROUNDING,
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
  auto *tileset = currentTileset_;
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
  auto *tileset = currentTileset_;
  if (!tileset) {
    ImGui::TextDisabled(ICON_FA_TRIANGLE_EXCLAMATION " Tileset not found");
    return;
  }

  if (tileset->getEntries().empty()) {
    ImGui::TextDisabled(ICON_FA_BOX_OPEN " No brushes in this tileset");
    return;
  }

  syncActiveBrushSelection();

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
  if (pendingSelectBrush_ || !pendingSelectBrushName_.empty()) {
    selectedIndices_.clear();
    bool found = false;

    // Search in filtered entries
    for (size_t i = 0; i < filteredEntries_.size() && !found; ++i) {
      if (isBrush(filteredEntries_[i].entry)) {
        const auto *brush = getBrush(filteredEntries_[i].entry);
        const bool pointerMatch =
            pendingSelectBrush_ != nullptr && brush == pendingSelectBrush_;
        const bool nameMatch =
            pendingSelectBrush_ == nullptr && brush != nullptr &&
            brush->getName() == pendingSelectBrushName_;
        if (brush && (pointerMatch || nameMatch)) {
          selectedIndices_.insert(static_cast<int>(i));
          selectedBrush_ = brush;
          found = true;
        }
      }
    }
    pendingSelectBrush_ = nullptr;
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

      ImVec2 tileSize(getIconSize(), getIconSize());
      ImVec2 cursorPos = ImGui::GetCursorScreenPos();

      const auto preview = getBrushPreview(brush);

      ImGui::InvisibleButton("##tile", tileSize);
      bool isHovered = ImGui::IsItemHovered();
      bool isClicked = ImGui::IsItemClicked();

      ImDrawList *dl = ImGui::GetWindowDrawList();

      const bool isSelected =
          selectedBrush_ == brush ||
          (brushController_ && brushController_->getCurrentBrush() == brush);

      auto [isPulsing, pulseElapsed] = computePulseState(brush);

      renderBrushCard(cursorPos, tileSize, preview, isSelected, isHovered,
                      isPulsing, pulseElapsed);

      // Tooltip
      if (isHovered) {
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

      // Click handling
      if (isClicked) {
        selectedBrush_ = brush;
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

      const auto preview = getBrushPreview(brush);

      // Drag source
      ImGui::InvisibleButton("##tile", tileSize);
      bool isHovered = ImGui::IsItemHovered();
      bool isClicked = ImGui::IsItemClicked();
      bool isSelected = selectedIndices_.count(static_cast<int>(entryIdx)) > 0 ||
                        selectedBrush_ == brush ||
                        (brushController_ &&
                         brushController_->getCurrentBrush() == brush);

      // Scroll to this brush if it's the target
      if ((scrollToBrush_ && brush == scrollToBrush_) ||
          (!scrollToBrush_ && !scrollToBrushName_.empty() &&
           brush->getName() == scrollToBrushName_)) {
        ImGui::SetScrollHereY(0.5f);
        scrollToBrush_ = nullptr;
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
          if (sourceIdx != fe.originalIndex && currentTileset_) {
            // Perform move
            currentTileset_->moveEntry(sourceIdx, fe.originalIndex);
            filterDirty_ = true;
            if (onTilesetModified_) {
              onTilesetModified_(*currentTileset_);
            }
          }
        }
        ImGui::EndDragDropTarget();
      }

      ImDrawList *dl = ImGui::GetWindowDrawList();

      // Render card with pulse animation support
      auto [isPulsing, pulseElapsed] = computePulseState(brush);
      renderBrushCard(cursorPos, tileSize, preview, isSelected, isHovered,
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

          selectedBrush_ = brush;
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
