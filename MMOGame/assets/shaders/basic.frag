#version 330 core

in vec4 v_Color;
in vec2 v_TexCoord;
in float v_TexIndex;

out vec4 FragColor;

uniform sampler2D u_Textures[32];

void main()
{
    vec4 texColor = vec4(1.0);
    
    // If texture index is -1, use solid color only
    if (v_TexIndex >= 0.0)
    {
        // Use a switch statement instead of dynamic indexing for compatibility
        int texIndex = int(v_TexIndex + 0.5); // Round to nearest int
        if (texIndex == 0) texColor = texture(u_Textures[0], v_TexCoord);
        else if (texIndex == 1) texColor = texture(u_Textures[1], v_TexCoord);
        else if (texIndex == 2) texColor = texture(u_Textures[2], v_TexCoord);
        else if (texIndex == 3) texColor = texture(u_Textures[3], v_TexCoord);
        // For colored quads, we typically don't use textures anyway
    }
    
    FragColor = v_Color * texColor;
}