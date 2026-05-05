#version 330 core

in vec3 v_FragPos;
in vec2 v_TexCoord;
in mat3 v_TBN;

out vec4 FragColor;

// Material textures
uniform sampler2D u_AlbedoMap;
uniform sampler2D u_NormalMap;
uniform bool u_UseNormalMap;

// Cascaded shadow map
uniform sampler2DArrayShadow u_ShadowMap;
uniform mat4 u_LightSpaceMatrices[4];
uniform float u_CascadeSplits[4];
uniform mat4 u_View;
uniform bool u_EnableShadows;
uniform float u_ShadowBias;
uniform bool u_ShowCascades;

// Light properties
uniform vec3 u_LightDir;
uniform vec3 u_LightColor;
uniform vec3 u_ViewPos;

// Ambient
uniform float u_AmbientStrength;

// Point lights
#define MAX_POINT_LIGHTS 32
uniform int u_PointLightCount;
uniform vec3 u_PointLightPos[MAX_POINT_LIGHTS];
uniform vec3 u_PointLightColor[MAX_POINT_LIGHTS];
uniform float u_PointLightRange[MAX_POINT_LIGHTS];

// Spot lights
#define MAX_SPOT_LIGHTS 8
uniform int u_SpotLightCount;
uniform vec3 u_SpotLightPos[MAX_SPOT_LIGHTS];
uniform vec3 u_SpotLightDir[MAX_SPOT_LIGHTS];
uniform vec3 u_SpotLightColor[MAX_SPOT_LIGHTS];
uniform float u_SpotLightRange[MAX_SPOT_LIGHTS];
uniform float u_SpotLightInnerCos[MAX_SPOT_LIGHTS];
uniform float u_SpotLightOuterCos[MAX_SPOT_LIGHTS];

float CalculateShadow(vec3 fragPos, vec3 normal, vec3 lightDir) {
    if (!u_EnableShadows) return 0.0;

    // Determine cascade by view-space depth
    float depth = abs((u_View * vec4(fragPos, 1.0)).z);
    int cascade = 3;
    for (int i = 0; i < 4; i++) {
        if (depth < u_CascadeSplits[i]) { cascade = i; break; }
    }

    // Transform to light space for selected cascade
    vec4 fragPosLightSpace = u_LightSpaceMatrices[cascade] * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 0.0;

    // Increase bias for farther cascades
    float bias = max(u_ShadowBias * (1.0 + float(cascade)) * (1.0 - dot(normal, lightDir)),
                     u_ShadowBias * 0.1);
    float currentDepth = projCoords.z - bias;

    // 3x3 PCF on texture array
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

vec3 CalcPointLights(vec3 normal, vec3 viewDir, vec3 albedo) {
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

        result += attenuation * (diff * albedo + spec * 0.3) * u_PointLightColor[i];
    }
    return result;
}

vec3 CalcSpotLights(vec3 normal, vec3 viewDir, vec3 albedo) {
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

        result += attenuation * spotFade * (diff * albedo + spec * 0.3) * u_SpotLightColor[i];
    }
    return result;
}

void main() {
    // Sample albedo texture
    vec3 albedo = texture(u_AlbedoMap, v_TexCoord).rgb;

    // Get normal - either from normal map or vertex normal
    vec3 normal;
    if (u_UseNormalMap) {
        // Sample normal map and transform from [0,1] to [-1,1]
        vec3 normalMap = texture(u_NormalMap, v_TexCoord).rgb;
        normalMap = normalMap * 2.0 - 1.0;
        // Transform normal from tangent space to world space
        normal = normalize(v_TBN * normalMap);
    } else {
        // Use vertex normal (third column of TBN matrix)
        normal = normalize(v_TBN[2]);
    }

    // View direction
    vec3 viewDir = normalize(u_ViewPos - v_FragPos);

    // Directional light
    vec3 lightDir = normalize(-u_LightDir);

    // Calculate shadow
    float shadow = CalculateShadow(v_FragPos, normal, lightDir);

    // Ambient
    vec3 ambient = u_AmbientStrength * albedo;

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * u_LightColor * albedo;

    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = spec * u_LightColor * 0.3;

    // Point and spot lights
    vec3 pointContrib = CalcPointLights(normal, viewDir, albedo);
    vec3 spotContrib = CalcSpotLights(normal, viewDir, albedo);

    // Apply shadow to diffuse and specular (not ambient), add local lights
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular) + pointContrib + spotContrib;

    FragColor = vec4(lighting, 1.0);

    // Cascade debug visualization
    if (u_ShowCascades) {
        float depth = abs((u_View * vec4(v_FragPos, 1.0)).z);
        vec3 cascadeColor;
        if (depth < u_CascadeSplits[0]) cascadeColor = vec3(1, 0, 0);       // Red
        else if (depth < u_CascadeSplits[1]) cascadeColor = vec3(0, 1, 0);  // Green
        else if (depth < u_CascadeSplits[2]) cascadeColor = vec3(0, 0, 1);  // Blue
        else cascadeColor = vec3(1, 1, 0);                                   // Yellow
        FragColor = vec4(cascadeColor, 1.0);
    }
}
