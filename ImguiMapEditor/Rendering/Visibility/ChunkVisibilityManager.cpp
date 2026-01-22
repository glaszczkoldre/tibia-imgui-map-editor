#include "ChunkVisibilityManager.h"
#include "Domain/ChunkedMap.h"
#include <algorithm>
#include <cmath>

namespace MapEditor {
namespace Rendering {

void ChunkVisibilityManager::update(const Domain::ChunkedMap &map,
                                    const VisibleBounds &bounds, int8_t floor_z,
                                    float floor_offset) {
  // Clear previous results
  visible_chunks_.clear();
  chunk_buffer_.clear();

  // Query map for chunks in the visible bounds
  map.getVisibleChunks(bounds.start_x, bounds.start_y, bounds.end_x,
                       bounds.end_y, floor_z, chunk_buffer_);

  // Process each chunk
  visible_chunks_.reserve(chunk_buffer_.size());

  for (Domain::Chunk *chunk : chunk_buffer_) {
    addVisibleChunk(chunk, floor_offset, bounds);
  }
}

void ChunkVisibilityManager::addVisibleChunk(Domain::Chunk *chunk,
                                             float floor_offset,
                                             const VisibleBounds &bounds) {

  VisibleChunk vc;
  vc.chunk = chunk;

  // Calculate screen position with floor offset for parallax
  vc.screen_x = chunk->world_x * TILE_SIZE - floor_offset;
  vc.screen_y = chunk->world_y * TILE_SIZE - floor_offset;

  // Determine if chunk is fully within viewport (enables fast path)
  vc.fully_visible = chunk->world_x >= bounds.start_x &&
                     chunk->world_x + Domain::Chunk::SIZE <= bounds.end_x &&
                     chunk->world_y >= bounds.start_y &&
                     chunk->world_y + Domain::Chunk::SIZE <= bounds.end_y;

  visible_chunks_.push_back(vc);
}

} // namespace Rendering
} // namespace MapEditor
