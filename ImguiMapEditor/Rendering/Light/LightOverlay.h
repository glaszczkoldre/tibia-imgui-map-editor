#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstdint>

namespace MapEditor {
namespace Rendering {

class LightOverlay {
public:
    LightOverlay();
    ~LightOverlay();

    // Non-copyable
    LightOverlay(const LightOverlay&) = delete;
    LightOverlay& operator=(const LightOverlay&) = delete;

    bool initialize();

    // Use specific destination rect (viewport coordinates)
    void apply(uint32_t light_texture_id,
               const glm::vec4& dest_rect,  // x, y, width, height in pixels
               const glm::vec2& viewport_size);

private:
    bool createShader();
    bool createQuad();

    uint32_t shader_program_ = 0;
    uint32_t vao_ = 0;
    uint32_t vbo_ = 0;
    int uniform_texture_ = -1;
    int uniform_mvp_ = -1;
    bool initialized_ = false;
};

} // namespace Rendering
} // namespace MapEditor
