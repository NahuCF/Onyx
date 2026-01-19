#version 330 core

layout (location = 0) in vec2 a_Position;

out vec2 v_ScreenPos;

uniform vec2 u_ViewportSize;

void main()
{
    // Calculate screen position (0,0 is top-left, ViewportSize is bottom-right)
    // a_Position is in [-1, 1], convert to [0, ViewportSize]
    v_ScreenPos = (a_Position * 0.5 + 0.5) * u_ViewportSize;

    // Convert to clip space (matching tile shader)
    vec2 clipPos = (v_ScreenPos / u_ViewportSize) * 2.0 - 1.0;
    clipPos.y = -clipPos.y;  // Flip Y to match OpenGL clip space

    gl_Position = vec4(clipPos, 0.0, 1.0);
}
