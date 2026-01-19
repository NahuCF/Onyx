#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include "../Types/Types.h"

namespace MMO {

// ============================================================
// WRITE BUFFER - Serializes data to bytes
// ============================================================

class WriteBuffer {
public:
    WriteBuffer(size_t initialCapacity = 256);

    // Basic types
    void WriteU8(uint8_t value);
    void WriteU16(uint16_t value);
    void WriteU32(uint32_t value);
    void WriteU64(uint64_t value);
    void WriteI8(int8_t value);
    void WriteI16(int16_t value);
    void WriteI32(int32_t value);
    void WriteI64(int64_t value);
    void WriteF32(float value);
    void WriteF64(double value);
    void WriteBool(bool value);

    // Complex types
    void WriteString(const std::string& value);
    void WriteVec2(const Vec2& value);
    void WriteVec3(const Vec3& value);
    void WriteBytes(const void* data, size_t size);

    // Access
    const uint8_t* Data() const { return m_Data.data(); }
    size_t Size() const { return m_WritePos; }
    void Clear() { m_WritePos = 0; }

private:
    void EnsureCapacity(size_t additionalBytes);

    std::vector<uint8_t> m_Data;
    size_t m_WritePos;
};

// ============================================================
// READ BUFFER - Deserializes bytes to data
// ============================================================

class ReadBuffer {
public:
    ReadBuffer(const uint8_t* data, size_t size);
    ReadBuffer(const std::vector<uint8_t>& data);

    // Basic types
    uint8_t ReadU8();
    uint16_t ReadU16();
    uint32_t ReadU32();
    uint64_t ReadU64();
    int8_t ReadI8();
    int16_t ReadI16();
    int32_t ReadI32();
    int64_t ReadI64();
    float ReadF32();
    double ReadF64();
    bool ReadBool();

    // Complex types
    std::string ReadString();
    Vec2 ReadVec2();
    Vec3 ReadVec3();
    void ReadBytes(void* dest, size_t size);

    // Access
    bool HasData(size_t bytes) const { return m_ReadPos + bytes <= m_Size; }
    size_t RemainingBytes() const { return m_Size - m_ReadPos; }
    size_t Position() const { return m_ReadPos; }
    void Reset() { m_ReadPos = 0; }

private:
    const uint8_t* m_Data;
    size_t m_Size;
    size_t m_ReadPos;
};

} // namespace MMO
