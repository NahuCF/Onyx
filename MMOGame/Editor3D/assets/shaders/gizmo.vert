#version 330 core

layout (location = 0) in vec3 a_Position;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform vec3 u_Color;

out vec3 v_Color;

void main() {
    v_Color = u_Color;
    gl_Position = u_Projection * u_View * u_Model * vec4(a_Position, 1.0);
}
