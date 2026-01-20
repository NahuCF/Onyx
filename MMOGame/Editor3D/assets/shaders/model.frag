#version 330 core

in vec3 v_FragPos;
in vec2 v_TexCoord;
in mat3 v_TBN;
in vec4 v_FragPosLightSpace;

out vec4 FragColor;

// Material textures
uniform sampler2D u_AlbedoMap;
uniform sampler2D u_NormalMap;
uniform bool u_UseNormalMap;

// Shadow map
uniform sampler2DShadow u_ShadowMap;
uniform bool u_EnableShadows;
uniform float u_ShadowBias;

// Light properties
uniform vec3 u_LightDir;
uniform vec3 u_LightColor;
uniform vec3 u_ViewPos;

// Ambient
uniform float u_AmbientStrength;

float CalculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    if (!u_EnableShadows) return 0.0;

    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // Check if outside shadow map bounds
    if (projCoords.z > 1.0) return 0.0;

    // Calculate bias based on angle between normal and light direction
    float bias = max(u_ShadowBias * (1.0 - dot(normal, lightDir)), u_ShadowBias * 0.1);

    // PCF (Percentage Closer Filtering) for softer shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);

    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec3 sampleCoord = vec3(projCoords.xy + vec2(x, y) * texelSize, projCoords.z - bias);
            shadow += texture(u_ShadowMap, sampleCoord);
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
    float shadow = CalculateShadow(v_FragPosLightSpace, normal, lightDir);

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
}
