#version 330 core

in vec2 TextureCoords;
in float TextureIndex;
in vec4 Color;

out vec4 FragColor;

uniform sampler2D u_Textures;

void main()
{
	if(TextureIndex >= 0) 
	{
		FragColor = texture(u_Textures, TextureCoords) * Color;
	}
	else
	{
		FragColor = vec4(Color);
	}


	if (FragColor.a < 0.1f) {
        discard;
    }
}