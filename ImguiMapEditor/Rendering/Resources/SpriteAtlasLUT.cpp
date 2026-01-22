#include "Rendering/Resources/SpriteAtlasLUT.h"
#include "Rendering/Resources/TextureAtlas.h"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace MapEditor {
namespace Rendering {

SpriteAtlasLUT::SpriteAtlasLUT() = default;

SpriteAtlasLUT::~SpriteAtlasLUT() {
  if (texture_id_ != 0) {
    glDeleteTextures(1, &texture_id_);
  }
  if (buffer_id_ != 0) {
    glDeleteBuffers(1, &buffer_id_);
  }
}

SpriteAtlasLUT::SpriteAtlasLUT(SpriteAtlasLUT &&other) noexcept
    : buffer_id_(other.buffer_id_), texture_id_(other.texture_id_),
      cpu_data_(std::move(other.cpu_data_)), use_ssbo_(other.use_ssbo_),
      initialized_(other.initialized_) {
  other.buffer_id_ = 0;
  other.texture_id_ = 0;
  other.initialized_ = false;
}

SpriteAtlasLUT &SpriteAtlasLUT::operator=(SpriteAtlasLUT &&other) noexcept {
  if (this != &other) {
    if (texture_id_ != 0) {
      glDeleteTextures(1, &texture_id_);
    }
    if (buffer_id_ != 0) {
      glDeleteBuffers(1, &buffer_id_);
    }
    buffer_id_ = other.buffer_id_;
    texture_id_ = other.texture_id_;
    cpu_data_ = std::move(other.cpu_data_);
    use_ssbo_ = other.use_ssbo_;
    initialized_ = other.initialized_;

    other.buffer_id_ = 0;
    other.texture_id_ = 0;
    other.initialized_ = false;
  }
  return *this;
}

bool SpriteAtlasLUT::initialize() {
  if (initialized_) {
    return true;
  }

  // Check for SSBO support (OpenGL 4.3+)
  GLint major = 0, minor = 0;
  glGetIntegerv(GL_MAJOR_VERSION, &major);
  glGetIntegerv(GL_MINOR_VERSION, &minor);
  use_ssbo_ = (major > 4) || (major == 4 && minor >= 3);

  spdlog::info("SpriteAtlasLUT: OpenGL {}.{}, using {}", major, minor,
               use_ssbo_ ? "SSBO" : "TBO fallback");

  // Allocate CPU-side data
  cpu_data_.resize(MAX_SPRITES);

  // Create GPU buffer
  glGenBuffers(1, &buffer_id_);
  if (buffer_id_ == 0) {
    spdlog::error("SpriteAtlasLUT: Failed to create buffer");
    return false;
  }

  GLenum target = use_ssbo_ ? GL_SHADER_STORAGE_BUFFER : GL_TEXTURE_BUFFER;
  glBindBuffer(target, buffer_id_);

  // Allocate with GL_DYNAMIC_DRAW for frequent partial updates
  size_t buffer_size = cpu_data_.size() * sizeof(Entry);
  glBufferData(target, static_cast<GLsizeiptr>(buffer_size), nullptr,
               GL_DYNAMIC_DRAW);

  GLenum err = glGetError();
  if (err != GL_NO_ERROR) {
    spdlog::error("SpriteAtlasLUT: Buffer allocation failed ({})", err);
    glDeleteBuffers(1, &buffer_id_);
    buffer_id_ = 0;
    return false;
  }

  // For TBO fallback, create texture view
  if (!use_ssbo_) {
    glGenTextures(1, &texture_id_);
    if (texture_id_ == 0) {
      spdlog::error("SpriteAtlasLUT: Failed to create TBO texture");
      glDeleteBuffers(1, &buffer_id_);
      buffer_id_ = 0;
      return false;
    }
    glBindTexture(GL_TEXTURE_BUFFER, texture_id_);
    // Use RGBA32F to match Entry layout (8 floats = 2 vec4s per entry)
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, buffer_id_);
  }

  glBindBuffer(target, 0);

  initialized_ = true;
  spdlog::debug("SpriteAtlasLUT: Initialized with {} entry capacity ({} MB)",
                MAX_SPRITES, buffer_size / (1024 * 1024));
  return true;
}

void SpriteAtlasLUT::update(uint32_t sprite_id, const AtlasRegion &region) {
  if (!initialized_ || sprite_id >= MAX_SPRITES) {
    return;
  }

  Entry &e = cpu_data_[sprite_id];
  e.u_min = region.u_min;
  e.v_min = region.v_min;
  e.u_max = region.u_max;
  e.v_max = region.v_max;
  e.layer = static_cast<float>(region.atlas_index);
  e.valid = 1.0f;

  // Track dirty range
  dirty_start_ = std::min(dirty_start_, sprite_id);
  dirty_end_ = std::max(dirty_end_, sprite_id + 1);

  // Immediate upload for single sprite
  uploadEntry(sprite_id);
}

void SpriteAtlasLUT::updateBatch(
    const std::vector<std::pair<uint32_t, const AtlasRegion *>> &entries) {
  if (!initialized_ || entries.empty()) {
    return;
  }

  uint32_t min_id = UINT32_MAX;
  uint32_t max_id = 0;

  for (const auto &[sprite_id, region] : entries) {
    if (sprite_id >= MAX_SPRITES || !region) {
      continue;
    }

    Entry &e = cpu_data_[sprite_id];
    e.u_min = region->u_min;
    e.v_min = region->v_min;
    e.u_max = region->u_max;
    e.v_max = region->v_max;
    e.layer = static_cast<float>(region->atlas_index);
    e.valid = 1.0f;

    min_id = std::min(min_id, sprite_id);
    max_id = std::max(max_id, sprite_id);
  }

  if (min_id <= max_id) {
    uploadRange(min_id, max_id - min_id + 1);
  }
}

void SpriteAtlasLUT::markPlaceholder(uint32_t sprite_id) {
  if (!initialized_ || sprite_id >= MAX_SPRITES) {
    return;
  }

  Entry &e = cpu_data_[sprite_id];
  e.valid = 0; // Shader will use fallback
  uploadEntry(sprite_id);
}

void SpriteAtlasLUT::bind(uint32_t binding_point) const {
  if (!initialized_) {
    return;
  }

  if (use_ssbo_) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_point, buffer_id_);
  } else {
    glActiveTexture(GL_TEXTURE0 + binding_point);
    glBindTexture(GL_TEXTURE_BUFFER, texture_id_);
  }
}

void SpriteAtlasLUT::clear() {
  if (!initialized_) {
    return;
  }

  // Reset all entries to invalid
  for (auto &entry : cpu_data_) {
    entry = Entry{};
  }

  // Upload entire buffer
  GLenum target = use_ssbo_ ? GL_SHADER_STORAGE_BUFFER : GL_TEXTURE_BUFFER;
  glBindBuffer(target, buffer_id_);
  glBufferSubData(target, 0,
                  static_cast<GLsizeiptr>(cpu_data_.size() * sizeof(Entry)),
                  cpu_data_.data());
  glBindBuffer(target, 0);

  dirty_start_ = UINT32_MAX;
  dirty_end_ = 0;
}

void SpriteAtlasLUT::uploadEntry(uint32_t sprite_id) {
  if (!initialized_ || sprite_id >= MAX_SPRITES) {
    return;
  }

  GLenum target = use_ssbo_ ? GL_SHADER_STORAGE_BUFFER : GL_TEXTURE_BUFFER;
  glBindBuffer(target, buffer_id_);
  glBufferSubData(target, static_cast<GLintptr>(sprite_id * sizeof(Entry)),
                  sizeof(Entry), &cpu_data_[sprite_id]);
  glBindBuffer(target, 0);
}

void SpriteAtlasLUT::uploadRange(uint32_t start_id, uint32_t count) {
  if (!initialized_ || start_id >= MAX_SPRITES) {
    return;
  }

  count = std::min(count, MAX_SPRITES - start_id);

  GLenum target = use_ssbo_ ? GL_SHADER_STORAGE_BUFFER : GL_TEXTURE_BUFFER;
  glBindBuffer(target, buffer_id_);
  glBufferSubData(target, static_cast<GLintptr>(start_id * sizeof(Entry)),
                  static_cast<GLsizeiptr>(count * sizeof(Entry)),
                  &cpu_data_[start_id]);
  glBindBuffer(target, 0);
}

} // namespace Rendering
} // namespace MapEditor
