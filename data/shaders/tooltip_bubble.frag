#version 330 core

in vec2 vLocalPos;      // Position within bubble (0-1)
in vec2 vBubbleSize;    // Width, height in pixels
in vec4 vBgColor;
in vec4 vBorderColor;
in float vRounding;

out vec4 FragColor;

// Signed distance function for rounded rectangle
float roundedRectSDF(vec2 pos, vec2 halfSize, float radius) {
    vec2 q = abs(pos) - halfSize + radius;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
}

void main() {
    // Convert from 0-1 to -1 to 1 centered coordinates
    vec2 centered = (vLocalPos - 0.5) * 2.0;
    vec2 pixelPos = centered * vBubbleSize * 0.5;
    vec2 halfSize = vBubbleSize * 0.5;
    
    // Calculate SDF distance
    float dist = roundedRectSDF(pixelPos, halfSize, vRounding);
    
    // Anti-aliased edges (1 pixel transition)
    float borderWidth = 1.0;
    
    // Inside fill
    float fillAlpha = 1.0 - smoothstep(-1.0, 0.0, dist);
    
    // Border band
    float borderAlpha = 1.0 - smoothstep(0.0, borderWidth, abs(dist));
    
    // Blend colors
    vec4 color = vBgColor * fillAlpha;
    color = mix(color, vBorderColor, borderAlpha * step(dist, 0.0));
    
    if (color.a < 0.01) discard;
    
    FragColor = color;
}
