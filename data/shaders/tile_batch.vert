#version 330 core
#extension GL_ARB_shader_storage_buffer_object : enable

// Per-vertex attributes (unit quad)
layout (location = 0) in vec2 aPos;       // 0,0 to 1,1
layout (location = 1) in vec2 aTexCoord;  // 0,0 to 1,1

// Per-instance attributes (TileInstance format)
layout (location = 2) in vec4 aRect;      // x, y, w, h
layout (location = 3) in uint aSpriteId;  // Sprite ID for LUT lookup
layout (location = 4) in vec4 aTint;      // r, g, b, a
layout (location = 5) in uint aFlags;     // Animation frame, selection state

// Sprite LUT entry structure (matches SpriteAtlasLUT::Entry)
struct SpriteLUTEntry {
    vec4 uv;        // u_min, v_min, u_max, v_max
    vec4 meta;      // layer, valid, pad, pad
};

// SSBO path (GL 4.3+)
#ifdef GL_ARB_shader_storage_buffer_object
layout(std430, binding = 0) buffer SpriteLUT {
    SpriteLUTEntry entries[];
};
#endif

// TBO fallback path (GL 3.3)
uniform samplerBuffer uSpriteLUT;
uniform int uUseSSBO;

out vec2 TexCoord;
out vec4 Tint;
flat out float Layer;
flat out float Valid;

uniform mat4 uMVP;

void main() {
    // Transform unit quad to screen position
    vec2 pos = aRect.xy + aPos * aRect.zw;
    gl_Position = uMVP * vec4(pos, 0.0, 1.0);

    // Lookup UV from sprite LUT
    vec4 uvData;
    vec4 metaData;
    
    if (uUseSSBO == 1) {
#ifdef GL_ARB_shader_storage_buffer_object
        SpriteLUTEntry entry = entries[aSpriteId];
        uvData = entry.uv;
        metaData = entry.meta;
#else
        // Fallback if extension not available
        uvData = vec4(0.0, 0.0, 1.0, 1.0);
        metaData = vec4(0.0, 0.0, 0.0, 0.0);
#endif
    } else {
        // TBO fallback: each entry is 2 vec4s (8 floats = 32 bytes)
        int baseIndex = int(aSpriteId) * 2;
        uvData = texelFetch(uSpriteLUT, baseIndex);
        metaData = texelFetch(uSpriteLUT, baseIndex + 1);
    }
    
    // Interpolate UV within the atlas region
    TexCoord = mix(uvData.xy, uvData.zw, aTexCoord);
    Tint = aTint;
    Layer = metaData.x;      // Texture array layer
    Valid = metaData.y;      // 1.0 if valid, 0.0 for placeholder
}
