#version 330 core

in vec4 color;

uniform vec2 u_MouseC;

out vec4 FragColor;

void main() 
{
    FragColor = vec4(color);
}