#include "LightOverlay.h"
#include <glad/glad.h>
#include <spdlog/spdlog.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace MapEditor {
namespace Rendering {

// Vertex shader with MVP support
static const char* VERTEX_SHADER = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

uniform mat4 uMVP;

out vec2 TexCoord;

void main() {
    gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

// Fragment shader - multiply blend
static const char* FRAGMENT_SHADER = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uLightTexture;

void main() {
    FragColor = texture(uLightTexture, TexCoord);
}
)";

LightOverlay::LightOverlay() = default;

LightOverlay::~LightOverlay() {
    if (shader_program_ != 0) {
        glDeleteProgram(shader_program_);
        shader_program_ = 0;
    }
    // RAII handles cleanup for vao_ and vbo_ automatically via DeferredVAOHandle/DeferredVBOHandle
}

bool LightOverlay::initialize() {
    if (initialized_) return true;
    
    if (!createShader()) {
        spdlog::error("LightOverlay: Failed to create shader");
        return false;
    }
    
    if (!createQuad()) {
        spdlog::error("LightOverlay: Failed to create quad");
        return false;
    }
    
    initialized_ = true;
    return true;
}

bool LightOverlay::createShader() {
    // Compile vertex shader
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &VERTEX_SHADER, nullptr);
    glCompileShader(vertex_shader);
    
    GLint success;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(vertex_shader, 512, nullptr, info_log);
        spdlog::error("LightOverlay vertex shader error: {}", info_log);
        glDeleteShader(vertex_shader);
        return false;
    }
    
    // Compile fragment shader
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &FRAGMENT_SHADER, nullptr);
    glCompileShader(fragment_shader);
    
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(fragment_shader, 512, nullptr, info_log);
        spdlog::error("LightOverlay fragment shader error: {}", info_log);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return false;
    }
    
    // Link program
    shader_program_ = glCreateProgram();
    glAttachShader(shader_program_, vertex_shader);
    glAttachShader(shader_program_, fragment_shader);
    glLinkProgram(shader_program_);
    
    glGetProgramiv(shader_program_, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(shader_program_, 512, nullptr, info_log);
        spdlog::error("LightOverlay shader link error: {}", info_log);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        glDeleteProgram(shader_program_);
        shader_program_ = 0;
        return false;
    }
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    uniform_texture_ = glGetUniformLocation(shader_program_, "uLightTexture");
    uniform_mvp_ = glGetUniformLocation(shader_program_, "uMVP");
    
    return true;
}

bool LightOverlay::createQuad() {
    // Unit Quad: 0,0 to 1,1
    float vertices[] = {
        // Position     // TexCoord
        0.0f, 0.0f,     0.0f, 0.0f,  // Top-Left
        1.0f, 0.0f,     1.0f, 0.0f,  // Top-Right
        1.0f, 1.0f,     1.0f, 1.0f,  // Bottom-Right
        
        0.0f, 0.0f,     0.0f, 0.0f,  // Top-Left
        1.0f, 1.0f,     1.0f, 1.0f,  // Bottom-Right
        0.0f, 1.0f,     0.0f, 1.0f   // Bottom-Left
    };
    
    vao_.create();
    vbo_.create();
    
    glBindVertexArray(vao_.get());
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo_.get());
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // TexCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    return vao_.isValid() && vbo_.isValid();
}

void LightOverlay::apply(uint32_t light_texture_id,
                         const glm::vec4& dest_rect,
                         const glm::vec2& viewport_size)
{
    if (!initialized_ || shader_program_ == 0 || light_texture_id == 0) return;
    
    // Enable multiply blend mode statelessly without calling glGet* driver queries
    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    
    // Use shader
    glUseProgram(shader_program_);
    
    // Calculate MVP matrix
    // Projection: Ortho(0, w, h, 0) - Top-Left origin
    glm::mat4 projection = glm::ortho(0.0f, viewport_size.x, viewport_size.y, 0.0f);
    
    // Model: Translate(x, y) * Scale(w, h)
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(dest_rect.x, dest_rect.y, 0.0f));
    model = glm::scale(model, glm::vec3(dest_rect.z, dest_rect.w, 1.0f));
    
    glm::mat4 mvp = projection * model;
    
    if (uniform_mvp_ >= 0) {
        glUniformMatrix4fv(uniform_mvp_, 1, GL_FALSE, glm::value_ptr(mvp));
    }
    
    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, light_texture_id);
    if (uniform_texture_ >= 0) {
        glUniform1i(uniform_texture_, 0);
    }
    
    // Draw quad
    glBindVertexArray(vao_.get());
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    // Restore state to standard alpha blending
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

} // namespace Rendering
} // namespace MapEditor
