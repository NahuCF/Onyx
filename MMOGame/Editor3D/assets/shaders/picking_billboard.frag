#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D u_IconTexture;  // Icon texture with alpha
uniform int u_ObjectID;           // GUID (lower 16 bits)
uniform int u_ObjectType;         // WorldObjectType enum
uniform int u_UseTexture;         // Whether to sample texture for alpha (0 or 1)

void main() {
    // If using texture, check alpha and discard transparent pixels
    if (u_UseTexture != 0) {
        float alpha = texture(u_IconTexture, TexCoord).a;
        if (alpha < 0.5)
            discard;
    }

    // Encode IDs into color channels (same format as picking.frag)
    // R + G = Object ID (16 bits)
    // B = Mesh Index low 8 bits (255 for no mesh)
    // A = Upper 4 bits: Mesh Index high 4 bits (15 for no mesh), Lower 4 bits: Object Type
    float r = float((u_ObjectID >> 8) & 0xFF) / 255.0;
    float g = float(u_ObjectID & 0xFF) / 255.0;
    float b = 1.0;  // 255 = low 8 bits of mesh index (4095 = no mesh)
    int meshHigh = 0x0F;  // 15 = high 4 bits of mesh index (4095 = no mesh)
    int typeLow = u_ObjectType & 0x0F;
    float a = float((meshHigh << 4) | typeLow) / 255.0;
    FragColor = vec4(r, g, b, a);
}
