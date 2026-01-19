#version 330 core

in vec3 v_NearPoint;
in vec3 v_FarPoint;
in mat4 v_View;
in mat4 v_Projection;

out vec4 FragColor;

uniform float u_GridScale;
uniform float u_GridFadeStart;
uniform float u_GridFadeEnd;

void main() {
    // Simple test - just output magenta
    FragColor = vec4(1.0, 0.0, 1.0, 0.5);
}
