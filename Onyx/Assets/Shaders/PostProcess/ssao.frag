#version 330 core

in vec2 v_TexCoord;
out float FragColor;

uniform sampler2D u_DepthTexture;
uniform sampler2D u_NoiseTexture;

uniform vec3 u_Samples[64];
uniform mat4 u_Projection;
uniform mat4 u_InvProjection;
uniform vec2 u_NoiseScale; // screen dimensions / noise size (4)
uniform int u_KernelSize;
uniform float u_Radius;
uniform float u_Bias;
uniform float u_Intensity;

vec3 ReconstructViewPos(vec2 uv, float depth) {
    vec4 ndc = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewPos = u_InvProjection * ndc;
    return viewPos.xyz / viewPos.w;
}

void main() {
    float depth = texture(u_DepthTexture, v_TexCoord).r;

    if (depth >= 1.0) {
        FragColor = 1.0;
        return;
    }

    vec3 fragPos = ReconstructViewPos(v_TexCoord, depth);

    // Reconstruct normal from depth derivatives
    vec3 dPdx = dFdx(fragPos);
    vec3 dPdy = dFdy(fragPos);
    vec3 normal = normalize(cross(dPdx, dPdy));

    // Sample noise for random rotation
    vec3 randomVec = texture(u_NoiseTexture, v_TexCoord * u_NoiseScale).xyz;

    // Gram-Schmidt to build TBN
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < u_KernelSize; i++) {
        vec3 samplePos = fragPos + TBN * u_Samples[i] * u_Radius;

        vec4 offset = u_Projection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float sampleDepth = texture(u_DepthTexture, offset.xy).r;
        vec3 sampleViewPos = ReconstructViewPos(offset.xy, sampleDepth);

        float rangeCheck = smoothstep(0.0, 1.0, u_Radius / abs(fragPos.z - sampleViewPos.z));
        occlusion += (sampleViewPos.z >= samplePos.z + u_Bias ? 1.0 : 0.0) * rangeCheck;
    }

    // Use pow() for intensity — tightens AO without amplifying noise
    float ao = 1.0 - (occlusion / float(u_KernelSize));
    FragColor = clamp(pow(ao, u_Intensity), 0.0, 1.0);
}
