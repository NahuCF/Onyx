#version 450 core
#extension GL_ARB_shader_draw_parameters : require

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoord;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_Bitangent;
layout (location = 5) in ivec4 a_BoneIds;
layout (location = 6) in vec4 a_BoneWeights;

out vec3 v_FragPos;
out vec2 v_TexCoord;
out mat3 v_TBN;

struct DrawData {
    mat4 model;
    int boneOffset;
    int boneCount;
    int _pad0;
    int _pad1;
};

layout(std430, binding = 0) readonly buffer DrawDataBuffer {
    DrawData draws[];
};

layout(std430, binding = 1) readonly buffer BoneBuffer {
    mat4 bones[];
};

uniform mat4 u_View;
uniform mat4 u_Projection;

void main() {
    DrawData d = draws[gl_DrawIDARB];

    mat4 boneTransform = mat4(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < 4; i++) {
        int boneId = a_BoneIds[i];
        float weight = a_BoneWeights[i];
        if (boneId >= 0 && boneId < d.boneCount && weight > 0.0) {
            boneTransform += bones[d.boneOffset + boneId] * weight;
            totalWeight += weight;
        }
    }

    if (totalWeight < 0.01) {
        boneTransform = mat4(1.0);
    }

    vec4 skinnedPos = boneTransform * vec4(a_Position, 1.0);
    vec4 worldPos = d.model * skinnedPos;
    v_FragPos = worldPos.xyz;
    v_TexCoord = a_TexCoord;

    mat3 skinMatrix = mat3(d.model * boneTransform);
    vec3 T = normalize(skinMatrix * a_Tangent);
    vec3 B = normalize(skinMatrix * a_Bitangent);
    vec3 N = normalize(skinMatrix * a_Normal);
    v_TBN = mat3(T, B, N);

    gl_Position = u_Projection * u_View * worldPos;
}
