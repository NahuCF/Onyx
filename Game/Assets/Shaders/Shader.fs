#version 330 core

out vec4 color;

uniform vec2 u_MouseC;

void main()
{
	vec2 st = gl_FragCoord.xy/800;
	color = vec4(st.x, st.y, 0.0f, 1.0f);
}