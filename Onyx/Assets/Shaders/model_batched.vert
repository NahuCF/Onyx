#version 450 core
#extension GL_ARB_shader_draw_parameters : require

// v2 MeshVertex: 28 B layout — see Onyx/Source/Graphics/Mesh.h
layout (location = 0) in vec3  a_Position;
layout (location = 1) in vec2  a_OctNormal;      // snorm16x2 -> [-1,1]
layout (location = 2) in vec2  a_TexCoord;       // half2
layout (location = 3) in vec2  a_OctTangent;     // snorm16x2 -> [-1,1]
layout (location = 4) in vec2  a_BitangentSign;  // snorm16x2; .x ~ +/-1, .y reserved

out vec3 v_FragPos;
out vec2 v_TexCoord;
out mat3 v_TBN;

struct DrawData {
    mat4 model;
};

layout(std430, binding = 0) readonly buffer DrawDataBuffer {
    DrawData draws[];
};

uniform mat4 u_View;
uniform mat4 u_Projection;

// Octahedral decode (Cigolle et al., JCGT 2014). Inverse of OctEncodeNormal in Mesh.h.
vec3 OctDecode(vec2 e) {
    vec3 v = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
    if (v.z < 0.0) {
        vec2 s = vec2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
        v.xy = (1.0 - abs(v.yx)) * s;
    }
    return normalize(v);
}

void main() {
    mat4 model = draws[gl_DrawIDARB].model;

    v_FragPos = vec3(model * vec4(a_Position, 1.0));
    v_TexCoord = a_TexCoord;

    vec3 N = OctDecode(a_OctNormal);
    vec3 T = OctDecode(a_OctTangent);
    vec3 B = cross(N, T) * sign(a_BitangentSign.x);

    mat3 normalMatrix = mat3(transpose(inverse(model)));
    v_TBN = mat3(normalize(normalMatrix * T),
                 normalize(normalMatrix * B),
                 normalize(normalMatrix * N));

    gl_Position = u_Projection * u_View * vec4(v_FragPos, 1.0);
}
