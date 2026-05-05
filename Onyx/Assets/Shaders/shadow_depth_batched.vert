#version 450 core
#extension GL_ARB_shader_draw_parameters : require

layout (location = 0) in vec3 a_Position;

struct DrawData {
    mat4 model;
};

layout(std430, binding = 0) readonly buffer DrawDataBuffer {
    DrawData draws[];
};

uniform mat4 u_LightSpaceMatrix;

void main() {
    mat4 model = draws[gl_DrawIDARB].model;
    gl_Position = u_LightSpaceMatrix * model * vec4(a_Position, 1.0);
}
