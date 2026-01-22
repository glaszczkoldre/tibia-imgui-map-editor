#version 330 core

// Per-vertex attributes (unit quad)
layout (location = 0) in vec2 aPos;       // 0,0 to 1,1

// Per-instance attributes
layout (location = 1) in vec4 aRect;      // x, y, w, h (screen position and size)
layout (location = 2) in vec4 aColor;     // background color RGBA
layout (location = 3) in vec4 aBorderColor; // border color RGBA
layout (location = 4) in float aRounding; // corner rounding radius

out vec2 vLocalPos;      // Position within bubble (0-1)
out vec2 vBubbleSize;    // Width, height in pixels
out vec4 vBgColor;
out vec4 vBorderColor;
out float vRounding;

uniform mat4 uMVP;

void main() {
    // Transform unit quad to screen position
    vec2 pos = aRect.xy + aPos * aRect.zw;
    gl_Position = uMVP * vec4(pos, 0.0, 1.0);
    
    // Pass to fragment shader for rounded rectangle SDF
    vLocalPos = aPos;
    vBubbleSize = aRect.zw;
    vBgColor = aColor;
    vBorderColor = aBorderColor;
    vRounding = aRounding;
}
