#include "RenderTarget.h"
#include <glad/glad.h>

namespace MapEditor {
namespace Rendering {

bool RenderTarget::resize(int width, int height) {
  if (width <= 0 || height <= 0) {
    return false;
  }

  // Skip if dimensions unchanged and already valid
  if (width == width_ && height == height_ && framebuffer_.isValid()) {
    return true;
  }

  // Resize framebuffer
  if (!framebuffer_.resize(width, height)) {
    return false;
  }

  // Update dimensions
  width_ = width;
  height_ = height;

  // Update projection matrix (orthographic, origin top-left)
  projection_ = glm::ortho(0.0f, static_cast<float>(width),
                           static_cast<float>(height), 0.0f, -1.0f, 1.0f);

  // View matrix stays identity for 2D
  view_ = glm::mat4(1.0f);

  return true;
}

void RenderTarget::bind() {
  framebuffer_.bind();
  glViewport(0, 0, width_, height_);
}

void RenderTarget::unbind() { framebuffer_.unbind(); }

GLuint RenderTarget::getTextureId() const {
  return framebuffer_.getTextureId();
}

bool RenderTarget::isValid() const {
  return framebuffer_.isValid() && width_ > 0 && height_ > 0;
}

void RenderTarget::beginPass(const glm::vec4 &clear_color) {
  bind();
  glDisable(GL_SCISSOR_TEST);
  glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
  glClear(GL_COLOR_BUFFER_BIT);
}

void RenderTarget::enableBlending() {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void RenderTarget::disableBlending() { glDisable(GL_BLEND); }

} // namespace Rendering
} // namespace MapEditor
