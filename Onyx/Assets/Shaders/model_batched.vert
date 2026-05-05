#version 450 core
#extension GL_ARB_shader_draw_parameters : require

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoord;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_Bitangent;

out vec3 v_FragPos;
out vec2 v_TexCoord;
out mat3 v_TBN;

struct DrawData {
    mat4 model;
};

layout(std430, binding = 0) readonly buffer DrawDataBuffer {
    DrawData draws[];
};

uniform mat4 u_View;
uniform mat4 u_Projection;

void main() {
    mat4 model = draws[gl_DrawIDARB].model;

    v_FragPos = vec3(model * vec4(a_Position, 1.0));
    v_TexCoord = a_TexCoord;

    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vec3 T = normalize(normalMatrix * a_Tangent);
    vec3 B = normalize(normalMatrix * a_Bitangent);
    vec3 N = normalize(normalMatrix * a_Normal);
    v_TBN = mat3(T, B, N);

    gl_Position = u_Projection * u_View * vec4(v_FragPos, 1.0);
}
