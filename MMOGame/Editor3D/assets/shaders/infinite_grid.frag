#version 330 core

in vec3 v_NearPoint;
in vec3 v_FarPoint;
in mat4 v_View;
in mat4 v_Projection;

out vec4 FragColor;

uniform float u_GridScale;
uniform float u_GridFadeStart;
uniform float u_GridFadeEnd;

float computeDepth(vec3 pos) {
    vec4 clipSpacePos = v_Projection * v_View * vec4(pos, 1.0);
    float clipDepth = clipSpacePos.z / clipSpacePos.w;
    return clipDepth * 0.5 + 0.5;
}

float getGrid(vec3 fragPos3D, float scale) {
    vec2 coord = fragPos3D.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    return 1.0 - min(line, 1.0);
}

void main() {
    vec3 rayDir = v_FarPoint - v_NearPoint;

    if (abs(rayDir.y) < 0.0001) {
        discard;
    }

    float t = -v_NearPoint.y / rayDir.y;

    if (t < 0.0) {
        discard;
    }

    vec3 fragPos3D = v_NearPoint + t * rayDir;

    // Calculate distance from camera for fading
    vec3 camPos = (inverse(v_View) * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    float dist = length(fragPos3D.xz - camPos.xz);

    // Fade out based on distance
    float fade = 1.0 - smoothstep(u_GridFadeStart, u_GridFadeEnd, dist);

    if (fade < 0.01) {
        discard;
    }

    // Small grid (1 unit) - subtle
    float smallGrid = getGrid(fragPos3D, u_GridScale);
    // Large grid (10 units) - more prominent
    float largeGrid = getGrid(fragPos3D, u_GridScale * 0.1);

    // Combine grids with different intensities
    float gridIntensity = smallGrid * 0.3 + largeGrid * 0.6;

    // Grid color - subtle gray
    vec3 gridColor = vec3(0.5);

    float alpha = gridIntensity * fade;

    if (alpha < 0.01) {
        discard;
    }

    // Write depth so objects can occlude the grid
    gl_FragDepth = computeDepth(fragPos3D);

    FragColor = vec4(gridColor, alpha);
}
