#version 330 core

in vec2 TexCoord;
in vec4 Tint;
flat in float Layer;
flat in float Valid;

out vec4 FragColor;

uniform sampler2DArray uTextureArray;
uniform vec4 uGlobalTint;
uniform vec4 uPlaceholderColor;  // Color for not-yet-loaded sprites

void main() {
    // Check if sprite is loaded (Valid = 1.0) or placeholder (Valid = 0.0)
    if (Valid < 0.5) {
        // Placeholder: render semi-transparent magenta
        FragColor = uPlaceholderColor * Tint * uGlobalTint;
        if (FragColor.a < 0.01)
            discard;
        return;
    }
    
    // Sample from texture array using layer index
    vec4 texColor = texture(uTextureArray, vec3(TexCoord, Layer));
    
    // Apply tint and global tint
    FragColor = texColor * Tint * uGlobalTint;
    
    // Discard fully transparent pixels
    if (FragColor.a < 0.01)
        discard;
}
