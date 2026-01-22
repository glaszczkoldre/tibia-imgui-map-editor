#version 330 core

layout (location = 0) in vec2 aTilePos;    // Tile X,Y in world space
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 uMVP;
uniform vec2 uCameraPos;     // Camera position in tile coords
uniform vec2 uViewportSize;  // Viewport width, height
uniform float uZoom;         // Zoom level
uniform float uTileSize;     // Base tile size (32)

void main() {
    // Transform tile coords to screen coords
    vec2 screenPos;
    screenPos.x = (aTilePos.x - uCameraPos.x) * uTileSize * uZoom + uViewportSize.x * 0.5;
    screenPos.y = (aTilePos.y - uCameraPos.y) * uTileSize * uZoom + uViewportSize.y * 0.5;
    
    gl_Position = uMVP * vec4(screenPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
