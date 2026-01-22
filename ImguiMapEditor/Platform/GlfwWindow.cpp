#include "GlfwWindow.h"
#include "Core/Config.h"

#include <glad/glad.h> // Must be included FIRST (provides all OpenGL declarations)
#define GLFW_INCLUDE_NONE // Prevent GLFW from including its own OpenGL headers
#include <GLFW/glfw3.h>
#include <cassert>
#include <spdlog/spdlog.h>

namespace MapEditor {
namespace Platform {

// Static instance for error callback access
GlfwWindow *GlfwWindow::s_instance_ = nullptr;

GlfwWindow::GlfwWindow() {
  assert(s_instance_ == nullptr && "Only one GlfwWindow instance allowed");
}

GlfwWindow::~GlfwWindow() { shutdown(); }

bool GlfwWindow::initialize(int width, int height, const char *title) {
  // Enforce default dimensions if config is invalid (0x0)
  if (width <= 0)
    width = Config::Window::DEFAULT_WIDTH;
  if (height <= 0)
    height = Config::Window::DEFAULT_HEIGHT;

  if (!initializeGLFW(width, height, title)) {
    return false;
  }
  if (!initializeOpenGL()) {
    return false;
  }
  return true;
}

bool GlfwWindow::initializeGLFW(int width, int height, const char *title) {
  s_instance_ = this; // Store for error callback

  glfwSetErrorCallback([](int error, const char *description) {
    // Ignore clipboard format conversion errors - common when Windows clipboard
    // contains non-text data (images, files, etc.). glfwGetClipboardString()
    // returns nullptr in this case, which our code already handles gracefully.
    // 65545 = GLFW_FORMAT_UNAVAILABLE
    if (error == 65545) {
      return;
    }

    spdlog::error("GLFW Error {}: {}", error, description);

    // Track display-related errors for graceful recovery
    // 65544 = GLFW_PLATFORM_ERROR for display settings query failure
    if (error == 65544 && s_instance_) {
      s_instance_->display_error_ = true;
    }
  });

  if (!glfwInit()) {
    spdlog::error("Failed to initialize GLFW");
    return false;
  }

  spdlog::debug("Attempting to create window: {}x{} Title: '{}'", width, height,
                title);

  // Try OpenGL 4.6 first (for modern features)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);

  // Fallback to OpenGL 3.3 if 4.6 is not available
  if (!window_) {
    spdlog::warn("OpenGL 4.6 not available, falling back to 3.3");

    glfwDefaultWindowHints(); // Reset hints to remove 4.6 specifics
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
  }

  if (!window_) {
    spdlog::error("Failed to create GLFW window");
    glfwTerminate();
    return false;
  }

  glfwMakeContextCurrent(window_);
  glfwSwapInterval(1); // VSync enabled

  spdlog::info("GLFW initialized, window created ({}x{})", width, height);
  return true;
}

bool GlfwWindow::initializeOpenGL() {
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    spdlog::error("Failed to initialize GLAD");
    return false;
  }

  gl_version_major_ = GLVersion.major;
  gl_version_minor_ = GLVersion.minor;

  spdlog::info("OpenGL {}.{} initialized", gl_version_major_,
               gl_version_minor_);
  spdlog::info("GL_VERSION string: {}", (const char *)glGetString(GL_VERSION));

  // MDI diagnostics
  spdlog::debug("GLAD_GL_VERSION_4_3 = {}", GLAD_GL_VERSION_4_3);
  spdlog::debug("glMultiDrawElementsIndirect = {}",
                (void *)glMultiDrawElementsIndirect);

  // Log renderer info
  const char *renderer = (const char *)glGetString(GL_RENDERER);
  const char *vendor = (const char *)glGetString(GL_VENDOR);
  spdlog::info("GPU: {} ({})", renderer ? renderer : "Unknown",
               vendor ? vendor : "Unknown");

  // Log available features
  if (gl_version_major_ >= 4) {
    spdlog::info("OpenGL 4.x features available:");
    if (gl_version_major_ > 4 || gl_version_minor_ >= 3) {
      spdlog::info("  - Multi-Draw Indirect (4.3+)");
    }
    if (gl_version_major_ > 4 || gl_version_minor_ >= 4) {
      spdlog::info("  - Persistent Mapped Buffers (4.4+)");
    }
    if (gl_version_major_ > 4 || gl_version_minor_ >= 5) {
      spdlog::info("  - Direct State Access (4.5+)");
    }
  } else {
    spdlog::info("Using OpenGL 3.3 compatibility mode");
  }

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  return true;
}

void GlfwWindow::shutdown() {
  if (window_) {
    glfwDestroyWindow(window_);
    window_ = nullptr;
  }
  s_instance_ = nullptr;
  glfwTerminate();
}

bool GlfwWindow::tryRecoverDisplay() {
  if (!display_error_) {
    return true; // No error to recover from
  }

  // Try to query framebuffer size - if it works, display is back
  if (window_) {
    int w, h;
    glfwGetFramebufferSize(window_, &w, &h);

    // If we got valid dimensions and no new error occurred, we're recovered
    if (w > 0 && h > 0 && !display_error_) {
      spdlog::info("Display recovered ({}x{})", w, h);
      return true;
    }

    // Reset the flag for next recovery attempt
    display_error_ = false;
  }

  return false;
}

void GlfwWindow::pollEvents() { glfwPollEvents(); }

void GlfwWindow::swapBuffers() {
  if (window_) {
    glfwSwapBuffers(window_);
  }
}

bool GlfwWindow::shouldClose() const {
  return window_ ? glfwWindowShouldClose(window_) : true;
}

void GlfwWindow::getSize(int &width, int &height) const {
  if (window_) {
    glfwGetWindowSize(window_, &width, &height);
  } else {
    width = height = 0;
  }
}

void GlfwWindow::getFramebufferSize(int &width, int &height) const {
  if (window_) {
    glfwGetFramebufferSize(window_, &width, &height);
  } else {
    width = height = 0;
  }
}

bool GlfwWindow::isMaximized() const {
  if (window_) {
    return glfwGetWindowAttrib(window_, GLFW_MAXIMIZED) != 0;
  }
  return false;
}

void GlfwWindow::setMaximized(bool maximized) {
  if (window_) {
    if (maximized) {
      glfwMaximizeWindow(window_);
    } else {
      glfwRestoreWindow(window_);
    }
  }
}

} // namespace Platform
} // namespace MapEditor
