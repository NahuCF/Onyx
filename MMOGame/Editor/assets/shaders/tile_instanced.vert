#version 330 core

// Per-vertex attributes (unit quad)
layout (location = 0) in vec2 a_Position;  // 0,0 to 1,1

// Per-instance attributes
layout (location = 1) in vec4 a_InstanceData;  // xy = world position (tile coords), zw = unused
layout (location = 2) in vec4 a_InstanceUV;    // xy = uv min, zw = uv max

out vec2 v_TexCoord;

uniform vec2 u_TileScreenSize;  // Tile size in screen pixels (after zoom)
uniform vec2 u_ViewportSize;    // Viewport dimensions
uniform vec2 u_CameraPos;       // Camera position in tile/world units
uniform float u_Zoom;           // Camera zoom
uniform vec2 u_TileSize;        // Base tile size in pixels (width, height)

void main()
{
    // Get world position from instance data (in tile units)
    vec2 worldPos = a_InstanceData.xy;

    // Apply camera offset
    vec2 relPos = worldPos - u_CameraPos;

    // Orthographic projection: simple multiply by tile size
    vec2 screenCenter = u_ViewportSize * 0.5;
    vec2 screenPos = screenCenter + relPos * u_TileSize * u_Zoom;

    // Offset to tile corner (screenPos is tile center)
    screenPos -= u_TileScreenSize * 0.5;

    // Add vertex offset within tile
    screenPos += a_Position * u_TileScreenSize;

    // Convert to clip space (0 to viewportSize -> -1 to 1)
    vec2 clipPos = (screenPos / u_ViewportSize) * 2.0 - 1.0;
    clipPos.y = -clipPos.y; // Flip Y for OpenGL

    // Interpolate UV based on vertex position
    v_TexCoord = mix(a_InstanceUV.xy, a_InstanceUV.zw, a_Position);

    gl_Position = vec4(clipPos, 0.0, 1.0);
}
