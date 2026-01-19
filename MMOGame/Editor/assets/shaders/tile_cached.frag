#version 330 core

in vec2 v_TexCoord;

out vec4 FragColor;

uniform sampler2D u_Texture;
uniform vec3 u_ColorKey;           // Color to treat as transparent (e.g., magenta)
uniform float u_ColorKeyThreshold; // How close to the key color to discard

void main()
{
    vec4 texColor = texture(u_Texture, v_TexCoord);

    // Check if this pixel matches the color key (magenta transparency)
    vec3 diff = abs(texColor.rgb - u_ColorKey);
    float maxDiff = max(max(diff.r, diff.g), diff.b);

    if (maxDiff < u_ColorKeyThreshold)
    {
        discard; // Transparent pixel
    }

    FragColor = texColor;
}
