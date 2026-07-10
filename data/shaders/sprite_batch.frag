#version 330 core

in vec2 TexCoord;
in vec4 Tint;
flat in float Layer;
out vec4 FragColor;

uniform sampler2DArray uTextureArray;
uniform vec4 uGlobalTint;

void main() {
    // Sample from texture array using layer index
    vec4 texColor = texture(uTextureArray, vec3(TexCoord, Layer));
    
    // Apply tint and global tint
    FragColor = texColor * Tint * uGlobalTint;
    
    // Discard fully transparent pixels
    if (FragColor.a < 0.01)
        discard;
}
