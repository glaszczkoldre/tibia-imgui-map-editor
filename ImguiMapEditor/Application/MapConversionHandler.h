#pragma once
#include "IO/Otbm/OtbmIdConverter.h"
#include <functional>
#include <string>

namespace MapEditor {

namespace Domain {
    class ChunkedMap;
}

namespace Services {
    class ClientDataService;
}

namespace AppLogic {

// Forward declaration
class MapTabManager;

// Re-use notification types (should match MapOperationHandler)
enum class ConversionNotificationType { Info, Success, Warning, Error };
using ConversionNotifyCallback = std::function<void(ConversionNotificationType, const std::string&)>;

/**
 * Handles map ID conversion operations (Server ID <-> Client ID).
 * Uses binary-level OTBM conversion - reads file, swaps IDs, writes back.
 */
class MapConversionHandler {
public:
    MapConversionHandler(
        MapTabManager& tab_manager,
        Services::ClientDataService* client_data,
        ConversionNotifyCallback notify_callback);
    
    void convertToServerId();
    void convertToClientId();
    
    void setClientData(Services::ClientDataService* client_data) {
        client_data_ = client_data;
    }

private:
    void executeConversion(
        IO::ConversionDirection direction,
        const std::string& suffix,
        const std::string& label);
    
    MapTabManager& tab_manager_;
    Services::ClientDataService* client_data_;
    ConversionNotifyCallback notify_;
};

} // namespace AppLogic
} // namespace MapEditor
