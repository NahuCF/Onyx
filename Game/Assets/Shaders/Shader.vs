#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 a_Color;

out vec4 color;

uniform mat4 move;

void main()
{
	color = a_Color;

	vec4 newPos = vec4(-1.0f, -1.0f, 0.0f, 0.0f);
	vec4 invertCoords = vec4(1.0f, -1.0f, 1.0f, 1.0f);
	gl_Position = invertCoords * (newPos + vec4(aPos, 1.0f));
}