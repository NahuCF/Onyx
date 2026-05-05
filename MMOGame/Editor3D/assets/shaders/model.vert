#version 330 core

// v2 MeshVertex: 28 B layout — see Onyx/Source/Graphics/Mesh.h
layout (location = 0) in vec3  a_Position;
layout (location = 1) in vec2  a_OctNormal;
layout (location = 2) in vec2  a_TexCoord;
layout (location = 3) in vec2  a_OctTangent;
layout (location = 4) in vec2  a_BitangentSign;

out vec3 v_FragPos;
out vec2 v_TexCoord;
out mat3 v_TBN;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

vec3 OctDecode(vec2 e) {
    vec3 v = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
    if (v.z < 0.0) {
        vec2 s = vec2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
        v.xy = (1.0 - abs(v.yx)) * s;
    }
    return normalize(v);
}

void main() {
    v_FragPos = vec3(u_Model * vec4(a_Position, 1.0));
    v_TexCoord = a_TexCoord;

    vec3 N = OctDecode(a_OctNormal);
    vec3 T = OctDecode(a_OctTangent);
    vec3 B = cross(N, T) * sign(a_BitangentSign.x);

    mat3 normalMatrix = mat3(transpose(inverse(u_Model)));
    v_TBN = mat3(normalize(normalMatrix * T),
                 normalize(normalMatrix * B),
                 normalize(normalMatrix * N));

    gl_Position = u_Projection * u_View * vec4(v_FragPos, 1.0);
}
