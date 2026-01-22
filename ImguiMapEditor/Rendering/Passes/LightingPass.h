#pragma once

#include "Rendering/Core/IRenderPass.h"

namespace MapEditor {
namespace Rendering {

/**
 * Renders the lighting layer/overlay.
 */
class LightingPass : public IRenderPass {
public:
  LightingPass() = default;
  ~LightingPass() override = default;

  void render(const RenderContext &context) override;

private:
  int last_start_floor_ = -1;
  int last_end_floor_ = -1;
};

} // namespace Rendering
} // namespace MapEditor
