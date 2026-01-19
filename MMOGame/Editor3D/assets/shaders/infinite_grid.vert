#version 330 core

layout(location = 0) in vec2 a_Position;

out vec3 v_NearPoint;
out vec3 v_FarPoint;
out mat4 v_View;
out mat4 v_Projection;

uniform mat4 u_View;
uniform mat4 u_Projection;

vec3 UnprojectPoint(float x, float y, float z, mat4 viewInv, mat4 projInv) {
    vec4 unprojectedPoint = viewInv * projInv * vec4(x, y, z, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

void main() {
    mat4 viewInv = inverse(u_View);
    mat4 projInv = inverse(u_Projection);

    // Unproject to get near and far points for ray
    v_NearPoint = UnprojectPoint(a_Position.x, a_Position.y, 0.0, viewInv, projInv);
    v_FarPoint = UnprojectPoint(a_Position.x, a_Position.y, 1.0, viewInv, projInv);

    v_View = u_View;
    v_Projection = u_Projection;

    gl_Position = vec4(a_Position, 0.0, 1.0);
}
