#version 330 core

layout (location = 0) in vec2 a_Position;
layout (location = 1) in vec2 a_TexCoord;

out vec2 v_TexCoord;

uniform vec2 u_ViewportPos;
uniform vec2 u_ViewportSize;
uniform vec2 u_WindowSize;
uniform vec2 u_TileScreenPos;
uniform vec2 u_TileScreenSize;

void main()
{
    // a_Position is in range [0, 1], scale to tile size and position
    vec2 screenPos = u_TileScreenPos + a_Position * u_TileScreenSize;

    // Convert to clip space
    vec2 clipPos = (screenPos / u_WindowSize) * 2.0 - 1.0;
    clipPos.y = -clipPos.y; // Flip Y for OpenGL

    v_TexCoord = a_TexCoord;
    gl_Position = vec4(clipPos, 0.0, 1.0);
}
