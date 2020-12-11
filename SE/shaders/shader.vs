#version 330 core

layout (location = 0) in vec3 aPos;

out vec4 colorsito;

uniform vec4 outColorsito;
uniform mat4 pos;
uniform vec4 escala;

void main()
{
	colorsito = outColorsito;
	gl_Position = pos * vec4(aPos, 1.0f);
}