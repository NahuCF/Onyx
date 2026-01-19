#version 330 core

layout (location = 0) in vec2 a_Position;

out vec2 v_ScreenPos;

uniform vec2 u_ViewportPos;
uniform vec2 u_ViewportSize;

void main()
{
    // a_Position is in range [-1, 1], convert to screen coordinates
    v_ScreenPos = u_ViewportPos + (a_Position * 0.5 + 0.5) * u_ViewportSize;
    gl_Position = vec4(a_Position, 0.0, 1.0);
}
