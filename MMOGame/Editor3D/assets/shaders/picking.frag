#version 330 core

out vec4 FragColor;

uniform int u_ObjectID;    // GUID (lower 16 bits)
uniform int u_MeshIndex;   // 0-4095 (12 bits), -1 or 4095 = whole object / no mesh
uniform int u_ObjectType;  // WorldObjectType enum (0-15)

void main() {
    // Encode IDs into color channels
    // R + G = Object ID (16 bits)
    // B = Mesh Index low 8 bits
    // A = Upper 4 bits: Mesh Index high 4 bits, Lower 4 bits: Object Type
    float r = float((u_ObjectID >> 8) & 0xFF) / 255.0;
    float g = float(u_ObjectID & 0xFF) / 255.0;
    float b = float(u_MeshIndex & 0xFF) / 255.0;
    int meshHigh = (u_MeshIndex >> 8) & 0x0F;  // Upper 4 bits of mesh index
    int typeLow = u_ObjectType & 0x0F;          // Lower 4 bits of object type
    float a = float((meshHigh << 4) | typeLow) / 255.0;
    FragColor = vec4(r, g, b, a);
}
