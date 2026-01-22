#pragma once
#include "Rendering/Core/Shader.h"
#include "Rendering/Core/GLHandle.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>
#include "../../Core/Config.h"

struct ImDrawList;

namespace MapEditor {
namespace Rendering {

/**
 * Per-bubble instance data for instanced rendering.
 * Contains position, size, colors, and shape parameters.
 */
struct BubbleInstance {
    float x, y;           // Screen position (top-left)
    float w, h;           // Size in pixels
    float bg_r, bg_g, bg_b, bg_a;       // Background color
    float border_r, border_g, border_b, border_a; // Border color
    float rounding;       // Corner radius
};

/**
 * Pending bubble with text for hybrid rendering.
 * Bubble shapes rendered via OpenGL, text via ImDrawList.
 */
struct PendingBubble {
    BubbleInstance instance;
    std::string text;
    float text_x, text_y;  // Text position in screen coords
    float max_text_width;
};

/**
 * High-performance batched tooltip bubble renderer using OpenGL instancing.
 * 
 * HYBRID APPROACH:
 * - Bubble backgrounds: OpenGL instanced quads with SDF shader
 * - Text: ImDrawList (text rendering is complex, leverages ImGui font atlas)
 * 
 * Usage:
 *   renderer.begin(projection, scale);
 *   for each tooltip:
 *       renderer.addBubble(x, y, text, is_waypoint);
 *   renderer.endBubbles();  // Issues OpenGL draw call for all bubbles
 *   renderer.renderText(draw_list);  // Renders text via ImGui
 */
class TooltipBubbleRenderer {
public:
    static constexpr size_t MAX_BUBBLES = Config::Tooltip::MAX_BUBBLES;

    TooltipBubbleRenderer();
    ~TooltipBubbleRenderer();

    // Non-copyable
    TooltipBubbleRenderer(const TooltipBubbleRenderer&) = delete;
    TooltipBubbleRenderer& operator=(const TooltipBubbleRenderer&) = delete;

    /**
     * Initialize GPU resources (shader, VAO, VBOs).
     * Must be called once before use.
     * @return true if successful
     */
    bool initialize();

    /**
     * Begin a new batch of bubbles.
     * @param projection Orthographic projection matrix
     * @param scale Current font scale for text sizing
     */
    void begin(const glm::mat4& projection, float scale);

    /**
     * Add a bubble to the batch.
     * @param screen_x, screen_y Top-left position of the tile in screen coords
     * @param tile_size Size of the tile in pixels (TILE_SIZE * zoom)
     * @param text Text content to display
     * @param is_waypoint If true, uses green color scheme
     */
    void addBubble(float screen_x, float screen_y, float tile_size,
                   const std::string& text, bool is_waypoint);

    /**
     * Render all bubble backgrounds via OpenGL.
     * Call this before renderText().
     */
    void endBubbles();

    /**
     * Render text for all bubbles via ImDrawList.
     * Call this after endBubbles().
     */
    void renderText(ImDrawList* draw_list);

    /**
     * Check if renderer is initialized and ready.
     */
    bool isInitialized() const { return initialized_; }

    /**
     * Get number of bubbles rendered in last batch.
     */
    size_t getBubbleCount() const { return pending_bubbles_.size(); }

private:
    std::unique_ptr<Shader> shader_;
    
    // OpenGL resources (RAII managed)
    DeferredVAOHandle vao_;
    DeferredVBOHandle quad_vbo_;      // Unit quad vertices (static)
    DeferredVBOHandle instance_vbo_;  // Instance data (dynamic)

    std::vector<PendingBubble> pending_bubbles_;
    std::vector<BubbleInstance> instances_;
    
    glm::mat4 projection_{1.0f};
    float current_scale_ = 1.0f;
    bool initialized_ = false;
    bool in_batch_ = false;
};

} // namespace Rendering
} // namespace MapEditor
