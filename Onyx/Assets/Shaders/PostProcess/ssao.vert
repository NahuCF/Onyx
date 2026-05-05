#version 330 core

layout(location = 0) in vec2 a_Position;

out vec2 v_TexCoord;

void main() {
    v_TexCoord = a_Position * 0.5 + 0.5;
    gl_Position = vec4(a_Position, 0.0, 1.0);
}
