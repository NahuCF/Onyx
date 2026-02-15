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

    // Apply shadow to diffuse and specular (not ambient)
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);

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
