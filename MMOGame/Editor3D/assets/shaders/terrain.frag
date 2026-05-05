#version 330 core

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoord;

out vec4 FragColor;

uniform sampler2D u_Heightmap;
uniform float u_HeightmapTexelSize;
uniform float u_ChunkSize;
uniform vec2 u_ChunkOrigin;

// Dual splatmaps (layers 0-3 and 4-7)
uniform sampler2D u_Splatmap0;
uniform sampler2D u_Splatmap1;

// Texture arrays (all materials)
uniform sampler2DArray u_DiffuseArray;
uniform sampler2DArray u_NormalArray;
uniform sampler2DArray u_RMAArray;

// Per-layer mapping: local layer index -> global array index
uniform int u_LayerArrayIndex[8];
uniform float u_LayerTiling[8];
uniform float u_LayerNormalStrength[8];

// PBR toggle
uniform int u_EnablePBR;

// Cascaded shadow map
uniform sampler2DArrayShadow u_ShadowMap;
uniform mat4 u_LightSpaceMatrices[4];
uniform float u_CascadeSplits[4];
uniform mat4 u_View;
uniform bool u_EnableShadows;
uniform float u_ShadowBias;

uniform vec3 u_LightDir;
uniform vec3 u_LightColor;
uniform vec3 u_ViewPos;
uniform float u_AmbientStrength;

uniform vec3 u_BrushPos;
uniform float u_BrushRadius;
uniform int u_ShowBrush;
uniform bool u_ShowCascades;
uniform int u_UsePixelNormals;
uniform int u_DebugSplatmap;  // 0=off, 1=weights RGBA, 2=weight sum, 3=chunk borders

// Point lights
#define MAX_POINT_LIGHTS 32
uniform int u_PointLightCount;
uniform vec3 u_PointLightPos[MAX_POINT_LIGHTS];
uniform vec3 u_PointLightColor[MAX_POINT_LIGHTS];   // color * intensity pre-multiplied
uniform float u_PointLightRange[MAX_POINT_LIGHTS];

// Spot lights
#define MAX_SPOT_LIGHTS 8
uniform int u_SpotLightCount;
uniform vec3 u_SpotLightPos[MAX_SPOT_LIGHTS];
uniform vec3 u_SpotLightDir[MAX_SPOT_LIGHTS];
uniform vec3 u_SpotLightColor[MAX_SPOT_LIGHTS];      // color * intensity pre-multiplied
uniform float u_SpotLightRange[MAX_SPOT_LIGHTS];
uniform float u_SpotLightInnerCos[MAX_SPOT_LIGHTS];
uniform float u_SpotLightOuterCos[MAX_SPOT_LIGHTS];

const float PI = 3.14159265359;

float CalculateShadow(vec3 fragPos, vec3 normal, vec3 lightDir) {
    if (!u_EnableShadows) return 0.0;

    float depth = abs((u_View * vec4(fragPos, 1.0)).z);
    int cascade = 3;
    for (int i = 0; i < 4; i++) {
        if (depth < u_CascadeSplits[i]) { cascade = i; break; }
    }

    vec4 fragPosLightSpace = u_LightSpaceMatrices[cascade] * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 0.0;

    float bias = max(u_ShadowBias * (1.0 + float(cascade)) * (1.0 - dot(normal, lightDir)),
                     u_ShadowBias * 0.1);
    float currentDepth = projCoords.z - bias;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(u_ShadowMap, 0).xy);
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec2 offset = vec2(x, y) * texelSize;
            shadow += texture(u_ShadowMap, vec4(projCoords.xy + offset, float(cascade), currentDepth));
        }
    }
    shadow /= 9.0;

    return 1.0 - shadow;
}

vec3 ComputePixelNormal() {
    float ts = u_HeightmapTexelSize;
    float paddedRes = 1.0 / ts;
    float interiorRes = paddedRes - 2.0;

    // Derive UV from world position for seamless normals across chunk boundaries
    vec2 localPos = v_FragPos.xz - u_ChunkOrigin;
    vec2 chunkUV = clamp(localPos / u_ChunkSize, 0.0, 1.0);
    vec2 paddedUV = (chunkUV * (interiorRes - 1.0) + 1.5) / paddedRes;

    float worldStep = u_ChunkSize / (interiorRes - 1.0);

    float h00 = texture(u_Heightmap, paddedUV + vec2(-ts, -ts)).r;
    float h10 = texture(u_Heightmap, paddedUV + vec2(  0, -ts)).r;
    float h20 = texture(u_Heightmap, paddedUV + vec2( ts, -ts)).r;
    float h01 = texture(u_Heightmap, paddedUV + vec2(-ts,   0)).r;
    float h21 = texture(u_Heightmap, paddedUV + vec2( ts,   0)).r;
    float h02 = texture(u_Heightmap, paddedUV + vec2(-ts,  ts)).r;
    float h12 = texture(u_Heightmap, paddedUV + vec2(  0,  ts)).r;
    float h22 = texture(u_Heightmap, paddedUV + vec2( ts,  ts)).r;

    float gx = -h00 + h20 - 2.0 * h01 + 2.0 * h21 - h02 + h22;
    float gz =  h00 + 2.0 * h10 + h20 - h02 - 2.0 * h12 - h22;

    return normalize(vec3(-gx, 8.0 * worldStep, -gz));
}

// ========== PBR BRDF Functions ==========

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / max(denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ========== Point/Spot Light Helpers ==========

vec3 CalcPointLightsBlinnPhong(vec3 normal, vec3 viewDir, vec3 albedo) {
    vec3 result = vec3(0.0);
    for (int i = 0; i < u_PointLightCount; i++) {
        vec3 toLight = u_PointLightPos[i] - v_FragPos;
        float dist = length(toLight);
        if (dist > u_PointLightRange[i]) continue;

        float attenuation = 1.0 - smoothstep(0.0, u_PointLightRange[i], dist);
        attenuation *= attenuation;
        vec3 L = toLight / dist;
        float diff = max(dot(normal, L), 0.0);

        vec3 H = normalize(L + viewDir);
        float spec = pow(max(dot(normal, H), 0.0), 32.0);

        result += attenuation * (diff * albedo + spec * 0.1) * u_PointLightColor[i];
    }
    return result;
}

vec3 CalcSpotLightsBlinnPhong(vec3 normal, vec3 viewDir, vec3 albedo) {
    vec3 result = vec3(0.0);
    for (int i = 0; i < u_SpotLightCount; i++) {
        vec3 toLight = u_SpotLightPos[i] - v_FragPos;
        float dist = length(toLight);
        if (dist > u_SpotLightRange[i]) continue;

        vec3 L = toLight / dist;
        float theta = dot(L, normalize(-u_SpotLightDir[i]));
        if (theta < u_SpotLightOuterCos[i]) continue;

        float epsilon = u_SpotLightInnerCos[i] - u_SpotLightOuterCos[i];
        float spotFade = clamp((theta - u_SpotLightOuterCos[i]) / max(epsilon, 0.001), 0.0, 1.0);

        float attenuation = 1.0 - smoothstep(0.0, u_SpotLightRange[i], dist);
        attenuation *= attenuation;
        float diff = max(dot(normal, L), 0.0);

        vec3 H = normalize(L + viewDir);
        float spec = pow(max(dot(normal, H), 0.0), 32.0);

        result += attenuation * spotFade * (diff * albedo + spec * 0.1) * u_SpotLightColor[i];
    }
    return result;
}

vec3 CalcPointLightsPBR(vec3 normal, vec3 viewDir, vec3 albedo, float roughness, float metallic, vec3 F0) {
    vec3 result = vec3(0.0);
    for (int i = 0; i < u_PointLightCount; i++) {
        vec3 toLight = u_PointLightPos[i] - v_FragPos;
        float dist = length(toLight);
        if (dist > u_PointLightRange[i]) continue;

        float attenuation = 1.0 - smoothstep(0.0, u_PointLightRange[i], dist);
        attenuation *= attenuation;
        vec3 L = toLight / dist;
        vec3 H = normalize(viewDir + L);

        float NDF = DistributionGGX(normal, H, roughness);
        float G = GeometrySmith(normal, viewDir, L, roughness);
        vec3 F = FresnelSchlick(max(dot(H, viewDir), 0.0), F0);

        vec3 specular = (NDF * G * F) /
            (4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, L), 0.0) + 0.0001);

        vec3 kD = (1.0 - F) * (1.0 - metallic);
        float NdotL = max(dot(normal, L), 0.0);

        result += attenuation * (kD * albedo / PI + specular) * u_PointLightColor[i] * NdotL;
    }
    return result;
}

vec3 CalcSpotLightsPBR(vec3 normal, vec3 viewDir, vec3 albedo, float roughness, float metallic, vec3 F0) {
    vec3 result = vec3(0.0);
    for (int i = 0; i < u_SpotLightCount; i++) {
        vec3 toLight = u_SpotLightPos[i] - v_FragPos;
        float dist = length(toLight);
        if (dist > u_SpotLightRange[i]) continue;

        vec3 L = toLight / dist;
        float theta = dot(L, normalize(-u_SpotLightDir[i]));
        if (theta < u_SpotLightOuterCos[i]) continue;

        float epsilon = u_SpotLightInnerCos[i] - u_SpotLightOuterCos[i];
        float spotFade = clamp((theta - u_SpotLightOuterCos[i]) / max(epsilon, 0.001), 0.0, 1.0);

        float attenuation = 1.0 - smoothstep(0.0, u_SpotLightRange[i], dist);
        attenuation *= attenuation;
        vec3 H = normalize(viewDir + L);

        float NDF = DistributionGGX(normal, H, roughness);
        float G = GeometrySmith(normal, viewDir, L, roughness);
        vec3 F = FresnelSchlick(max(dot(H, viewDir), 0.0), F0);

        vec3 specular = (NDF * G * F) /
            (4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, L), 0.0) + 0.0001);

        vec3 kD = (1.0 - F) * (1.0 - metallic);
        float NdotL = max(dot(normal, L), 0.0);

        result += attenuation * spotFade * (kD * albedo / PI + specular) * u_SpotLightColor[i] * NdotL;
    }
    return result;
}

void main() {
    // Remap UV [0,1] to splatmap texel centers [0.5/64, 63.5/64]
    vec2 splatUV = v_TexCoord * (63.0 / 64.0) + vec2(0.5 / 64.0);
    vec4 w0 = texture(u_Splatmap0, splatUV);
    vec4 w1 = texture(u_Splatmap1, splatUV);

    float weights[8] = float[8](w0.r, w0.g, w0.b, w0.a, w1.r, w1.g, w1.b, w1.a);

    // Sample diffuse per layer using texture arrays
    vec3 albedo = vec3(0.0);
    for (int i = 0; i < 8; i++) {
        if (weights[i] < 0.004) continue;
        vec2 uv = v_TexCoord * u_LayerTiling[i];
        albedo += weights[i] * texture(u_DiffuseArray, vec3(uv, float(u_LayerArrayIndex[i]))).rgb;
    }

    // Debug splatmap visualization
    if (u_DebugSplatmap == 1) {
        float borderX = min(v_TexCoord.x, 1.0 - v_TexCoord.x);
        float borderZ = min(v_TexCoord.y, 1.0 - v_TexCoord.y);
        float border = min(borderX, borderZ);
        FragColor = (border < 0.005)
            ? vec4(1.0, 1.0, 0.0, 1.0)
            : vec4(w0.r, w0.g, w0.b + w0.a * 0.5, 1.0);
        return;
    }
    if (u_DebugSplatmap == 2) {
        float sum = 0.0;
        for (int i = 0; i < 8; i++) sum += weights[i];
        float deviation = abs(sum - 1.0) * 10.0;
        float borderX = min(v_TexCoord.x, 1.0 - v_TexCoord.x);
        float borderZ = min(v_TexCoord.y, 1.0 - v_TexCoord.y);
        FragColor = (min(borderX, borderZ) < 0.005)
            ? vec4(1.0, 1.0, 0.0, 1.0)
            : vec4(deviation, sum, 0.0, 1.0);
        return;
    }
    if (u_DebugSplatmap == 3) {
        vec2 texelPos = splatUV * 64.0;
        vec2 texelFrac = fract(texelPos);
        float grid = smoothstep(0.0, 0.05, min(texelFrac.x, texelFrac.y)) *
                     smoothstep(0.0, 0.05, min(1.0 - texelFrac.x, 1.0 - texelFrac.y));
        float borderX = min(v_TexCoord.x, 1.0 - v_TexCoord.x);
        float borderZ = min(v_TexCoord.y, 1.0 - v_TexCoord.y);
        FragColor = (min(borderX, borderZ) < 0.005)
            ? vec4(1.0, 1.0, 0.0, 1.0)
            : vec4(vec3(grid) * vec3(w0.r, w0.g, w0.b + w0.a * 0.5), 1.0);
        return;
    }

    // Geometry normal from heightmap or vertex
    vec3 geometryNormal = (u_UsePixelNormals != 0) ? ComputePixelNormal() : normalize(v_Normal);
    vec3 normal = geometryNormal;
    vec3 lighting;

    if (u_EnablePBR != 0) {
        // Sample and blend normal maps per layer (with per-layer strength)
        vec3 blendedNormal = vec3(0.0);
        for (int i = 0; i < 8; i++) {
            if (weights[i] < 0.004) continue;
            vec2 uv = v_TexCoord * u_LayerTiling[i];
            vec3 nm = texture(u_NormalArray, vec3(uv, float(u_LayerArrayIndex[i]))).rgb * 2.0 - 1.0;
            nm.xy *= u_LayerNormalStrength[i];
            blendedNormal += weights[i] * nm;
        }
        blendedNormal = normalize(blendedNormal);

        // Construct TBN from geometry normal
        vec3 N = geometryNormal;
        vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
        vec3 T = normalize(cross(up, N));
        vec3 B = cross(N, T);
        mat3 TBN = mat3(T, B, N);

        normal = normalize(TBN * blendedNormal);

        // Sample RMA per layer
        vec3 rma = vec3(0.0);
        for (int i = 0; i < 8; i++) {
            if (weights[i] < 0.004) continue;
            vec2 uv = v_TexCoord * u_LayerTiling[i];
            rma += weights[i] * texture(u_RMAArray, vec3(uv, float(u_LayerArrayIndex[i]))).rgb;
        }
        float roughness = clamp(rma.r, 0.04, 1.0);
        float metallic = rma.g;
        float ao = rma.b;

        // Cook-Torrance BRDF
        vec3 viewDir = normalize(u_ViewPos - v_FragPos);
        vec3 lightDir = normalize(-u_LightDir);
        vec3 F0 = mix(vec3(0.04), albedo, metallic);
        vec3 H = normalize(viewDir + lightDir);

        float NDF = DistributionGGX(normal, H, roughness);
        float G = GeometrySmith(normal, viewDir, lightDir, roughness);
        vec3 F = FresnelSchlick(max(dot(H, viewDir), 0.0), F0);

        vec3 specular = (NDF * G * F) /
            (4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 0.0001);

        vec3 kD = (1.0 - F) * (1.0 - metallic);
        float NdotL = max(dot(normal, lightDir), 0.0);
        float shadow = CalculateShadow(v_FragPos, normal, lightDir);

        vec3 ambient = u_AmbientStrength * albedo * ao;
        vec3 Lo = (kD * albedo / PI + specular) * u_LightColor * NdotL;

        // Point and spot lights (PBR)
        vec3 pointLo = CalcPointLightsPBR(normal, viewDir, albedo, roughness, metallic, F0);
        vec3 spotLo = CalcSpotLightsPBR(normal, viewDir, albedo, roughness, metallic, F0);

        lighting = ambient + (1.0 - shadow) * Lo + pointLo + spotLo;

    } else {
        // Blinn-Phong path
        vec3 viewDir = normalize(u_ViewPos - v_FragPos);
        vec3 lightDir = normalize(-u_LightDir);

        float shadow = CalculateShadow(v_FragPos, normal, lightDir);

        vec3 ambient = u_AmbientStrength * albedo;
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * u_LightColor * albedo;

        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
        vec3 specular = spec * u_LightColor * 0.1;

        // Point and spot lights (Blinn-Phong)
        vec3 pointContrib = CalcPointLightsBlinnPhong(normal, viewDir, albedo);
        vec3 spotContrib = CalcSpotLightsBlinnPhong(normal, viewDir, albedo);

        lighting = ambient + (1.0 - shadow) * (diffuse + specular) + pointContrib + spotContrib;
    }

    // Brush overlay (shared by both paths)
    if (u_ShowBrush == 1) {
        float dist = length(v_FragPos.xz - u_BrushPos.xz);
        float ring = abs(dist - u_BrushRadius);
        float ringAlpha = 1.0 - smoothstep(0.0, 0.15, ring);
        lighting = mix(lighting, vec3(0.2, 1.0, 0.2), ringAlpha * 0.8);
        if (dist < u_BrushRadius) {
            float fillAlpha = 0.05 * (1.0 - dist / u_BrushRadius);
            lighting = mix(lighting, vec3(0.3, 1.0, 0.3), fillAlpha);
        }
    }

    FragColor = vec4(lighting, 1.0);

    // Cascade debug visualization (shared by both paths)
    if (u_ShowCascades) {
        float depth = abs((u_View * vec4(v_FragPos, 1.0)).z);
        vec3 cascadeColor;
        if (depth < u_CascadeSplits[0]) cascadeColor = vec3(1, 0, 0);
        else if (depth < u_CascadeSplits[1]) cascadeColor = vec3(0, 1, 0);
        else if (depth < u_CascadeSplits[2]) cascadeColor = vec3(0, 0, 1);
        else cascadeColor = vec3(1, 1, 0);
        FragColor = vec4(mix(lighting, cascadeColor, 0.15), 1.0);
    }
}
