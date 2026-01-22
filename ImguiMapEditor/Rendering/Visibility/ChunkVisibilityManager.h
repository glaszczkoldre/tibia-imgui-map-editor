#pragma once
#include "../../Core/Config.h"
#include "Rendering/Utils/MathUtils.h"
#include "Rendering/Visibility/VisibleBounds.h"
#include <cmath>
#include <cstdint>
#include <vector>


namespace MapEditor {

namespace Domain {
class ChunkedMap;
class Chunk;
} // namespace Domain

namespace Rendering {

/**
 * Represents a chunk and its screen position for rendering.
 */
struct VisibleChunk {
  Domain::Chunk *chunk = nullptr; // Non-owning observer pointer
  float screen_x = 0.0f; // Screen X position (with floor offset applied)
  float screen_y = 0.0f; // Screen Y position (with floor offset applied)
  bool fully_visible =
      false; // True if chunk is entirely within viewport (fast path)
};

/**
 * Determines which chunks are visible in the current viewport.
 * Handles culling, ordering, and screen position calculation.
 *
 * Extracted from MapRenderer to separate visibility logic from rendering.
 */
class ChunkVisibilityManager {
public:
  static constexpr float TILE_SIZE = Config::Rendering::TILE_SIZE;

  /**
   * Update visible chunks for a given floor.
   * This rebuilds the visibility list.
   * @param map The chunked map to query
   * @param bounds Visible tile bounds
   * @param floor_z Floor to query
   * @param floor_offset Pixel offset for parallax effect
   */
  void update(const Domain::ChunkedMap &map, const VisibleBounds &bounds,
              int8_t floor_z, float floor_offset = 0.0f);

  /**
   * Get the list of visible chunks after update().
   */
  const std::vector<VisibleChunk> &getVisibleChunks() const {
    return visible_chunks_;
  }

  /**
   * Get count of visible chunks.
   */
  size_t getVisibleChunkCount() const { return visible_chunks_.size(); }

  /**
   * Reserve capacity for expected chunk count (reduces allocations).
   */
  void reserve(size_t capacity) {
    visible_chunks_.reserve(capacity);
    chunk_buffer_.reserve(capacity);
  }

private:
  std::vector<VisibleChunk> visible_chunks_;
  std::vector<Domain::Chunk *> chunk_buffer_; // Reusable buffer for map query

  void addVisibleChunk(Domain::Chunk *chunk, float floor_offset,
                       const VisibleBounds &bounds);
};

} // namespace Rendering
} // namespace MapEditor
