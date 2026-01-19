#version 330 core

layout (location = 0) in vec2 aPos;       // Quad vertices (-0.5 to 0.5)
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 u_View;
uniform mat4 u_Projection;
uniform vec3 u_Position;      // World position of icon
uniform float u_Size;         // Icon size in world units

void main() {
    // Extract camera right and up vectors from view matrix
    vec3 cameraRight = vec3(u_View[0][0], u_View[1][0], u_View[2][0]);
    vec3 cameraUp = vec3(u_View[0][1], u_View[1][1], u_View[2][1]);

    // Billboard position - always faces camera
    vec3 worldPos = u_Position
                  + cameraRight * aPos.x * u_Size
                  + cameraUp * aPos.y * u_Size;

    gl_Position = u_Projection * u_View * vec4(worldPos, 1.0);
    TexCoord = aTexCoord;
}
