#version 330 core

in vec2 TextureCoords;

out vec4 FragColor;

uniform sampler2D TextureData;

void main()
{
	FragColor = texture(TextureData, TextureCoords);
}