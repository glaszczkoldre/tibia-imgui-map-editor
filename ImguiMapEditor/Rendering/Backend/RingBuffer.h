#pragma once

#include <glad/glad.h>
#include "Rendering/Core/GLHandle.h"
#include "Rendering/Core/SyncHandle.h"
#include "Core/Config.h"
#include <cstddef>
#include <cstdint>

namespace MapEditor {
namespace Rendering {

/**
 * Triple-buffered ring buffer with persistent mapping for zero-copy GPU uploads.
 * 
 * This eliminates CPU-GPU synchronization stalls by:
 * 1. Mapping the buffer ONCE at initialization (persistent mapping)
 * 2. Using 3 sections that rotate each frame (triple buffering)
 * 3. Using fence sync to ensure GPU is done with a section before reusing
 * 
 * Modern OpenGL (4.4+) with fallback to buffer orphaning for GL 3.3.
 */
class RingBuffer {
public:
    static constexpr size_t BUFFER_COUNT = Config::Performance::RING_BUFFER_COUNT;

    RingBuffer() = default;
    ~RingBuffer();

    // Non-copyable, but movable
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;
    RingBuffer(RingBuffer&& other) noexcept;
    RingBuffer& operator=(RingBuffer&& other) noexcept;

    /**
     * Initialize the ring buffer with persistent mapping.
     * @param element_size Size of each element in bytes
     * @param max_elements Maximum elements per section
     * @return true if successful
     */
    bool initialize(size_t element_size, size_t max_elements);

    /**
     * Release GPU resources.
     */
    void cleanup();

    /**
     * Wait for the current section to be available, return write pointer.
     * This will block if the GPU is still reading from this section.
     * @param count Number of elements to write (for bounds checking)
     * @return Pointer to write data, or nullptr if count exceeds capacity
     */
    void* waitAndMap(size_t count);

    /**
     * Signal that we've finished writing. Must call before drawing!
     * For fallback mode: unmaps the buffer.
     * For persistent mode: no-op (stays mapped).
     */
    void finishWrite();

    /**
     * Signal that we've finished drawing. Call after draw calls!
     * For persistent mode: inserts fence, advances section.
     * For fallback mode: no-op.
     */
    void signalFinished();

    /**
     * Get the OpenGL buffer ID.
     */
    GLuint getBufferId() const { return buffer_.get(); }

    /**
     * Get byte offset to current section for vertex attribute setup.
     */
    size_t getCurrentSectionOffset() const;

    /**
     * Get max elements per section.
     */
    size_t getMaxElements() const { return max_elements_; }

    /**
     * Check if using persistent mapping (GL 4.4+) or fallback.
     */
    bool isPersistentlyMapped() const { return use_persistent_mapping_; }

private:
    DeferredVBOHandle buffer_;
    void* mapped_ptr_ = nullptr;  // Persistently mapped (or nullptr for fallback)
    SyncHandle fences_[BUFFER_COUNT];
    
    size_t element_size_ = 0;
    size_t max_elements_ = 0;
    size_t section_size_ = 0;  // element_size * max_elements
    size_t current_section_ = 0;
    
    bool use_persistent_mapping_ = false;
    bool initialized_ = false;
};

} // namespace Rendering
} // namespace MapEditor
