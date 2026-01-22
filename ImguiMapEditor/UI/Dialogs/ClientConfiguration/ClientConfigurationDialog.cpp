#include "UI/Dialogs/ClientConfiguration/ClientConfigurationDialog.h"
#include "Domain/ClientVersion.h"
#include "Services/ClientVersionPersistence.h"
#include "Services/ClientVersionRegistry.h"
#include <IconsFontAwesome6.h>
#include <cstring>
#include <iomanip>
#include <nfd.hpp>
#include <spdlog/spdlog.h>
#include <sstream>

namespace MapEditor {
namespace UI {

void ClientConfigurationDialog::open(
    Services::ClientVersionRegistry &registry) {
  registry_ = &registry;
  is_open_ = true;
  selected_version_ = 0;
  filter_buffer_[0] = '\0';
  show_delete_confirmation_ = false;

  // Initialize extracted components
  table_widget_.setRegistry(registry_);
  table_widget_.setSelectionCallback(
      [this](uint32_t version) { selected_version_ = version; });
  table_widget_.setEditCallback([this](uint32_t version) {
    edit_modal_.openForEdit(version);
  });
  table_widget_.setDeleteCallback([this](uint32_t version) {
    version_to_delete_ = version;
    show_delete_confirmation_ = true;
  });

  details_card_.setRegistry(registry_);

  edit_modal_.setRegistry(registry_);
  edit_modal_.setCallbacks(
      [this]() { /* on save - could trigger refresh */ },
      [this](ClientEditData &) { /* browse handled by modal internally */ });

  // Select default version if set
  if (registry_->getDefaultVersion() > 0) {
    selected_version_ = registry_->getDefaultVersion();
  }
}

void ClientConfigurationDialog::close() { is_open_ = false; }

bool ClientConfigurationDialog::render() {
  if (!is_open_ || !registry_) {
    return false;
  }

  ImGuiIO &io = ImGui::GetIO();
  ImVec2 window_size(900, 600);
  ImVec2 window_pos((io.DisplaySize.x - window_size.x) * 0.5f,
                    (io.DisplaySize.y - window_size.y) * 0.5f);

  ImGui::SetNextWindowPos(window_pos, ImGuiCond_Appearing);
  ImGui::SetNextWindowSize(window_size, ImGuiCond_Appearing);
  ImGui::SetNextWindowSizeConstraints(ImVec2(700, 400),
                                      ImVec2(FLT_MAX, FLT_MAX));

  bool open = true;
  if (ImGui::Begin("Client Configuration", &open,
                   ImGuiWindowFlags_NoCollapse |
                       ImGuiWindowFlags_NoSavedSettings)) {

    // Header with action buttons
    if (ImGui::Button(ICON_FA_PLUS " Add", ImVec2(80, 0))) {
      edit_modal_.openForAdd();
    }

    ImGui::SameLine();

    // Edit button - enabled only when a client is selected
    bool can_edit = (selected_version_ != 0);
    if (!can_edit)
      ImGui::BeginDisabled();
    if (ImGui::Button(ICON_FA_PEN " Edit", ImVec2(80, 0))) {
      if (selected_version_ != 0) {
        edit_modal_.openForEdit(selected_version_);
      }
    }
    if (!can_edit)
      ImGui::EndDisabled();

    ImGui::SameLine();

    // Delete button - enabled only when a client is selected
    if (!can_edit)
      ImGui::BeginDisabled();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(0.7f, 0.3f, 0.3f, 1.0f));
    if (ImGui::Button(ICON_FA_TRASH " Delete", ImVec2(80, 0))) {
      if (selected_version_ != 0) {
        version_to_delete_ = selected_version_;
        show_delete_confirmation_ = true;
      }
    }
    ImGui::PopStyleColor(2);
    if (!can_edit)
      ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    ImGui::Text("Filter:");
    ImGui::SameLine();
    ImGui::PushItemWidth(120);
    ImGui::InputText("##filter", filter_buffer_, sizeof(filter_buffer_));
    ImGui::PopItemWidth();

    float save_btn_x = ImGui::GetContentRegionMax().x - 120;
    ImGui::SameLine(save_btn_x);
    if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save All", ImVec2(120, 0))) {
      Services::ClientVersionsData data;
      data.versions = registry_->getVersionsMap();
      data.otb_to_version = registry_->getOtbMapping();
      data.default_version = registry_->getDefaultVersion();

      if (Services::ClientVersionPersistence::saveToJson(
              registry_->getJsonPath(), data)) {
        if (onSave)
          onSave();
      }
    }

    ImGui::Separator();

    // Footer height: card(120) + separator(2) + close button row(30) = 152
    const float footer_total_height = 152.0f;
    float table_height = ImGui::GetContentRegionAvail().y - footer_total_height;
    if (table_height < 100.0f)
      table_height = 100.0f;

    // Version table - delegate to component
    table_widget_.setFilter(filter_buffer_);
    table_widget_.setSelectedVersion(selected_version_);
    table_widget_.render(table_height);
    selected_version_ = table_widget_.getSelectedVersion();

    // Footer section
    ImGui::Separator();

    // Selected version details - delegate to component
    details_card_.setSelectedVersion(selected_version_);
    details_card_.render();

    // Close button at bottom-right
    float close_btn_x = ImGui::GetContentRegionMax().x - 120;
    ImGui::SetCursorPosX(close_btn_x);
    if (ImGui::Button("Close", ImVec2(120, 0))) {
      is_open_ = false;
    }
  }
  ImGui::End();

  // Render modals - delegate to component
  edit_modal_.render();
  if (show_delete_confirmation_) {
    renderDeleteConfirmation();
  }

  if (!open) {
    is_open_ = false;
  }

  return is_open_;
}

void ClientConfigurationDialog::renderDeleteConfirmation() {
  ImGui::OpenPopup("Confirm Delete");

  if (ImGui::BeginPopupModal("Confirm Delete", &show_delete_confirmation_,
                             ImGuiWindowFlags_AlwaysAutoResize)) {

    auto *version = registry_->getVersion(version_to_delete_);
    if (version) {
      ImGui::Text(ICON_FA_TRIANGLE_EXCLAMATION
                  " Are you sure you want to delete:");
      ImGui::Text("%s (version %u)?", version->getName().c_str(),
                  version_to_delete_);
      ImGui::Spacing();
      ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f),
                         "This cannot be undone until you reload clients.json.");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Cancel", ImVec2(100, 0))) {
      show_delete_confirmation_ = false;
    }

    ImGui::SameLine(ImGui::GetWindowWidth() - 110);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("Delete", ImVec2(100, 0))) {
      registry_->removeClient(version_to_delete_);
      if (selected_version_ == version_to_delete_) {
        selected_version_ = 0;
      }
      show_delete_confirmation_ = false;
    }
    ImGui::PopStyleColor();

    ImGui::EndPopup();
  }
}

void ClientConfigurationDialog::browseForPath() {
  // Not currently used - modal handles its own browsing
}

} // namespace UI
} // namespace MapEditor
