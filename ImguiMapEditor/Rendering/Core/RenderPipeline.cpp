#include "Rendering/Core/RenderPipeline.h"

namespace MapEditor {
namespace Rendering {

void RenderPipeline::addPass(std::unique_ptr<IRenderPass> pass) {
  if (pass) {
    passes_.push_back(std::move(pass));
  }
}

void RenderPipeline::render(const RenderContext &context) {
  for (const auto &pass : passes_) {
    pass->render(context);
  }
}

void RenderPipeline::setLODMode(bool enabled) {
  for (const auto &pass : passes_) {
    pass->setLODMode(enabled);
  }
}

void RenderPipeline::clear() {
  passes_.clear();
}

size_t RenderPipeline::getPassCount() const {
  return passes_.size();
}

} // namespace Rendering
} // namespace MapEditor
