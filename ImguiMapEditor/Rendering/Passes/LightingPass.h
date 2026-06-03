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
};

} // namespace Rendering
} // namespace MapEditor
