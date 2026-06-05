#pragma once
#include <memory>

namespace MapEditor::Brushes {
class IBrush;
}

namespace MapEditor::Domain {
class ChunkedMap;
}

namespace MapEditor::Services {
class BrushSettingsService;
}

namespace MapEditor::Services::Preview {

class IPreviewProvider;

/**
 * Factory that creates preview providers based on brush type.
 */
class BrushPreviewFactory {
public:
  /**
   * Create appropriate preview provider for the given brush.
   * @param brush The brush to create a provider for
   * @param settings Brush settings for size/shape
   * @return Provider, or nullptr if brush has no preview support
   */
  std::unique_ptr<IPreviewProvider>
  createProvider(const ::MapEditor::Brushes::IBrush *brush,
                 BrushSettingsService *settings,
                 const ::MapEditor::Domain::ChunkedMap *map = nullptr);
};

} // namespace MapEditor::Services::Preview
