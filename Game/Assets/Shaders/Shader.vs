#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTextureCoords;
layout (location = 3) in float aTextureIndex;

out vec2 TextureCoords;
out float TextureIndex;
out vec4 Color;

uniform mat4 move;

void main()
{
    Color = aColor;
    TextureIndex = aTextureIndex;
	TextureCoords = aTextureCoords;
		
	vec4 newPos = vec4(-1.0f, -1.0f, 0.0f, 0.0f);
	vec4 invertCoords = vec4(1.0f, -1.0f, -1.0f, 1.0f);
	gl_Position = invertCoords * (newPos + vec4(aPos, 1.0f));
}