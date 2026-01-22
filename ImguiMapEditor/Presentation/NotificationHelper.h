#pragma once
#include "Application/MapOperationHandler.h"
#include "ImGuiNotify.hpp"
#include <string>

namespace MapEditor::Presentation {

/**
 * Maps internal notification types to ImGui toast types.
 */
inline ImGuiToastType
mapToToastType(AppLogic::MapOperationHandler::NotificationType type) {
  switch (type) {
  case AppLogic::MapOperationHandler::NotificationType::Success:
    return ImGuiToastType::Success;
  case AppLogic::MapOperationHandler::NotificationType::Error:
    return ImGuiToastType::Error;
  case AppLogic::MapOperationHandler::NotificationType::Warning:
    return ImGuiToastType::Warning;
  case AppLogic::MapOperationHandler::NotificationType::Info:
  default:
    return ImGuiToastType::Info;
  }
}

/**
 * Shows a notification toast with direct ImGuiToastType.
 * This is the primary overload used by UI code.
 */
inline void showNotification(ImGuiToastType type, const std::string &message,
                             int duration_ms = 3000) {
  ImGui::InsertNotification({type, duration_ms, "%s", message.c_str()});
}

/**
 * Shows a notification toast with the appropriate type.
 * @param type The notification type from MapOperationHandler
 * @param message The message to display
 * @param duration_ms Toast display duration in milliseconds (default: 3000)
 */
inline void
showNotification(AppLogic::MapOperationHandler::NotificationType type,
                 const std::string &message, int duration_ms = 3000) {
  showNotification(mapToToastType(type), message, duration_ms);
}

/**
 * Shows a notification toast from raw type integer.
 * @param type The notification type as integer
 * @param message The message to display
 * @param duration_ms Toast display duration in milliseconds (default: 3000)
 */
inline void showNotification(int type, const std::string &message,
                             int duration_ms = 3000) {
  showNotification(
      static_cast<AppLogic::MapOperationHandler::NotificationType>(type),
      message, duration_ms);
}

// Convenience functions for common notification types
inline void showSuccess(const std::string &message, int duration_ms = 3000) {
  showNotification(ImGuiToastType::Success, message, duration_ms);
}

inline void showError(const std::string &message, int duration_ms = 5000) {
  showNotification(ImGuiToastType::Error, message, duration_ms);
}

inline void showWarning(const std::string &message, int duration_ms = 3000) {
  showNotification(ImGuiToastType::Warning, message, duration_ms);
}

inline void showInfo(const std::string &message, int duration_ms = 2000) {
  showNotification(ImGuiToastType::Info, message, duration_ms);
}

} // namespace MapEditor::Presentation
