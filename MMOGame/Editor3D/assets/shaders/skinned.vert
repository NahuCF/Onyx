#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoords;
layout(location = 3) in vec3 a_Tangent;
layout(location = 4) in vec3 a_Bitangent;
layout(location = 5) in ivec4 a_BoneIds;
layout(location = 6) in vec4 a_BoneWeights;

out vec3 v_FragPos;
out vec2 v_TexCoord;
out mat3 v_TBN;
out float v_ClipSpaceZ;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

const int MAX_BONES = 100;
uniform mat4 u_BoneMatrices[MAX_BONES];

void main() {
    // Calculate skinned position
    mat4 boneTransform = mat4(0.0);

    for (int i = 0; i < 4; i++) {
        if (a_BoneIds[i] >= 0 && a_BoneIds[i] < MAX_BONES) {
            boneTransform += u_BoneMatrices[a_BoneIds[i]] * a_BoneWeights[i];
        }
    }

    // If no bones affect this vertex, use identity
    float totalWeight = a_BoneWeights.x + a_BoneWeights.y + a_BoneWeights.z + a_BoneWeights.w;
    if (totalWeight < 0.01) {
        boneTransform = mat4(1.0);
    }

    vec4 skinnedPosition = boneTransform * vec4(a_Position, 1.0);

    vec4 worldPos = u_Model * skinnedPosition;
    v_FragPos = worldPos.xyz;

    v_TexCoord = a_TexCoords;

    // Calculate TBN matrix for normal mapping
    mat3 skinMatrix = mat3(u_Model * boneTransform);
    vec3 T = normalize(skinMatrix * a_Tangent);
    vec3 B = normalize(skinMatrix * a_Bitangent);
    vec3 N = normalize(skinMatrix * a_Normal);
    v_TBN = mat3(T, B, N);

    // Calculate clip space position
    vec4 viewPos = u_View * worldPos;
    gl_Position = u_Projection * viewPos;

    // Pass clip space Z for cascade selection
    v_ClipSpaceZ = -viewPos.z;
}
