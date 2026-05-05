#version 330 core

in vec2 v_TexCoord;
out vec4 FragColor;

uniform sampler2D u_SceneColor;
uniform sampler2D u_SSAOBlurred;

void main() {
    vec3 color = texture(u_SceneColor, v_TexCoord).rgb;
    float ao = texture(u_SSAOBlurred, v_TexCoord).r;
    FragColor = vec4(color * ao, 1.0);
}
