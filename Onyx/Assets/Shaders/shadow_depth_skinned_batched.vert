#version 450 core
#extension GL_ARB_shader_draw_parameters : require

layout (location = 0) in vec3 a_Position;
layout (location = 5) in ivec4 a_BoneIds;
layout (location = 6) in vec4 a_BoneWeights;

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

uniform mat4 u_LightSpaceMatrix;

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
    if (totalWeight < 0.01) boneTransform = mat4(1.0);

    vec4 skinnedPos = boneTransform * vec4(a_Position, 1.0);
    gl_Position = u_LightSpaceMatrix * d.model * skinnedPos;
}
