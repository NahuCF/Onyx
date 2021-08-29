#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 a_Color;

out vec4 color;

uniform mat4 move;

void main()
{
	color = a_Color;

	gl_Position = move * vec4(aPos, 1.0f);
}