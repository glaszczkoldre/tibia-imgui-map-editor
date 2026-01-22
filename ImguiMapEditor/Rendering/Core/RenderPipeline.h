#pragma once

#include <memory>
#include <vector>

#include "Rendering/Core/IRenderPass.h"

namespace MapEditor {
namespace Rendering {

/**
 * Manages the sequence of rendering passes.
 * Separates the orchestration of render passes from the MapRenderer.
 */
class RenderPipeline {
public:
  RenderPipeline() = default;
  ~RenderPipeline() = default;

  RenderPipeline(const RenderPipeline &) = delete;
  RenderPipeline &operator=(const RenderPipeline &) = delete;
  RenderPipeline(RenderPipeline &&) = default;
  RenderPipeline &operator=(RenderPipeline &&) = default;

  /**
   * Add a render pass to the pipeline.
   * Passes are executed in the order they are added.
   */
  void addPass(std::unique_ptr<IRenderPass> pass);

  /**
   * Execute all passes in the pipeline.
   */
  void render(const RenderContext &context);

  /**
   * Set LOD mode for all passes.
   */
  void setLODMode(bool enabled);

  /**
   * Clear all passes.
   */
  void clear();

  /**
   * Get the number of passes in the pipeline.
   */
  size_t getPassCount() const;

private:
  std::vector<std::unique_ptr<IRenderPass>> passes_;
};

} // namespace Rendering
} // namespace MapEditor
