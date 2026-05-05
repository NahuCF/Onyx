#version 330 core

in vec2 v_TexCoord;
out float FragColor;

uniform sampler2D u_SSAOInput;
uniform sampler2D u_DepthTexture;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(u_SSAOInput, 0));
    float centerDepth = texture(u_DepthTexture, v_TexCoord).r;
    float result = 0.0;
    float totalWeight = 0.0;

    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            vec2 sampleUV = v_TexCoord + offset;
            float sampleDepth = texture(u_DepthTexture, sampleUV).r;

            // Smooth bilateral weight instead of hard cutoff
            float depthDiff = abs(centerDepth - sampleDepth);
            float depthWeight = exp(-depthDiff * depthDiff / (2.0 * 0.0001));

            // Spatial Gaussian weight
            float dist2 = float(x * x + y * y);
            float spatialWeight = exp(-dist2 / 4.5);

            float w = depthWeight * spatialWeight;
            result += texture(u_SSAOInput, sampleUV).r * w;
            totalWeight += w;
        }
    }

    FragColor = result / max(totalWeight, 0.001);
}
