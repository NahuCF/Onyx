#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTextureCoords;

out vec2 TextureCoords;

uniform mat4 move;

void main()
{
	TextureCoords = aTextureCoords;
	gl_Position =  move * vec4(aPos, 1.0f);
}