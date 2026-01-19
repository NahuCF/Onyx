#version 330 core

in vec2 v_ScreenPos;

out vec4 FragColor;

uniform vec2 u_ViewportPos;
uniform vec2 u_ViewportSize;
uniform vec2 u_CameraPos;
uniform float u_Zoom;
uniform float u_TileSize;
uniform float u_ChunkSize;
uniform vec4 u_GridColor;
uniform vec4 u_ChunkColor;
uniform float u_LineWidth;

// Convert screen position to isometric world position
vec2 screenToWorld(vec2 screenPos)
{
    // Get position relative to viewport center
    vec2 viewportCenter = u_ViewportPos + u_ViewportSize * 0.5;
    vec2 relPos = (screenPos - viewportCenter) / u_Zoom;

    // Isometric tile dimensions (2:1 ratio)
    float halfTileW = u_TileSize;
    float halfTileH = u_TileSize * 0.5;

    // Reverse isometric projection
    float worldX = (relPos.x / halfTileW + relPos.y / halfTileH) * 0.5 + u_CameraPos.x;
    float worldY = (relPos.y / halfTileH - relPos.x / halfTileW) * 0.5 + u_CameraPos.y;

    return vec2(worldX, worldY);
}

void main()
{
    vec2 worldPos = screenToWorld(v_ScreenPos);

    // Calculate distance to nearest grid line
    // Grid lines are at integer X and Y values in world space
    float distToXLine = abs(fract(worldPos.x + 0.5) - 0.5);
    float distToYLine = abs(fract(worldPos.y + 0.5) - 0.5);

    // Convert distance from world units to screen pixels for consistent line width
    float pixelsPerUnit = u_Zoom * u_TileSize;
    float lineWidthWorld = u_LineWidth / pixelsPerUnit;

    // Calculate line intensity with anti-aliasing
    float xLineIntensity = 1.0 - smoothstep(0.0, lineWidthWorld, distToXLine);
    float yLineIntensity = 1.0 - smoothstep(0.0, lineWidthWorld, distToYLine);
    float lineIntensity = max(xLineIntensity, yLineIntensity);

    // Check for chunk borders
    float distToChunkX = abs(fract(worldPos.x / u_ChunkSize + 0.5) - 0.5) * u_ChunkSize;
    float distToChunkY = abs(fract(worldPos.y / u_ChunkSize + 0.5) - 0.5) * u_ChunkSize;

    float chunkLineWidth = lineWidthWorld * 2.0; // Chunk lines are thicker
    float chunkXIntensity = 1.0 - smoothstep(0.0, chunkLineWidth, distToChunkX);
    float chunkYIntensity = 1.0 - smoothstep(0.0, chunkLineWidth, distToChunkY);
    float chunkIntensity = max(chunkXIntensity, chunkYIntensity);

    // Combine colors
    vec4 gridColorFinal = u_GridColor * lineIntensity;
    vec4 chunkColorFinal = u_ChunkColor * chunkIntensity;

    // Chunk lines override regular grid lines
    FragColor = mix(gridColorFinal, chunkColorFinal, chunkIntensity);

    // Discard fully transparent pixels
    if (FragColor.a < 0.01)
        discard;
}
