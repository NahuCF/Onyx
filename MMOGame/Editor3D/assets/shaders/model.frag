#version 330 core

in vec3 v_FragPos;
in vec2 v_TexCoord;
in mat3 v_TBN;

out vec4 FragColor;

// Material textures
uniform sampler2D u_AlbedoMap;
uniform sampler2D u_NormalMap;
uniform bool u_UseNormalMap;

// Light properties
uniform vec3 u_LightDir;
uniform vec3 u_LightColor;
uniform vec3 u_ViewPos;

// Ambient
uniform float u_AmbientStrength;

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

    // Ambient
    vec3 ambient = u_AmbientStrength * albedo;

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * u_LightColor * albedo;

    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = spec * u_LightColor * 0.3;

    // Final color
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
