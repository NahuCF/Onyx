#version 330 core

// Per-vertex attributes
layout (location = 0) in vec2 a_WorldPos;   // Tile center in world/tile units
layout (location = 1) in vec2 a_Corner;     // Corner offset (0,0), (1,0), (1,1), (0,1)
layout (location = 2) in vec2 a_TexCoord;   // UV coordinates

out vec2 v_TexCoord;

uniform vec2 u_ViewportSize;
uniform vec2 u_TileScreenSize;  // Tile size in screen pixels
uniform vec2 u_CameraPos;       // Camera position in tile units
uniform float u_Zoom;
uniform vec2 u_TileSize;        // Tile size (width, height) in pixels

void main()
{
    // Apply camera offset before isometric projection
    vec2 relPos = a_WorldPos - u_CameraPos;

    // Isometric projection using configured tile dimensions
    float halfTileW = u_TileSize.x * 0.5;
    float halfTileH = u_TileSize.y * 0.5;

    float isoX = (relPos.x - relPos.y) * halfTileW;
    float isoY = (relPos.x + relPos.y) * halfTileH;

    // Apply zoom and center
    vec2 screenCenter = u_ViewportSize * 0.5;
    vec2 screenPos = screenCenter + vec2(isoX, isoY) * u_Zoom;

    // Offset to corner
    // Corner (0,0) = top-left, (1,0) = top-right, (1,1) = bottom-right, (0,1) = bottom-left
    vec2 cornerOffset = (a_Corner - 0.5) * u_TileScreenSize;
    screenPos += cornerOffset;

    // Convert to clip space
    vec2 clipPos = (screenPos / u_ViewportSize) * 2.0 - 1.0;
    clipPos.y = -clipPos.y;

    v_TexCoord = a_TexCoord;
    gl_Position = vec4(clipPos, 0.0, 1.0);
}
