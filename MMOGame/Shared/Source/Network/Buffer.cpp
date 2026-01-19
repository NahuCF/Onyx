#include "Buffer.h"
#include <stdexcept>

namespace MMO {

// ============================================================
// WRITE BUFFER IMPLEMENTATION
// ============================================================

WriteBuffer::WriteBuffer(size_t initialCapacity)
    : m_Data(initialCapacity), m_WritePos(0) {
}

void WriteBuffer::EnsureCapacity(size_t additionalBytes) {
    size_t required = m_WritePos + additionalBytes;
    if (required > m_Data.size()) {
        size_t newSize = m_Data.size() * 2;
        while (newSize < required) {
            newSize *= 2;
        }
        m_Data.resize(newSize);
    }
}

void WriteBuffer::WriteU8(uint8_t value) {
    EnsureCapacity(1);
    m_Data[m_WritePos++] = value;
}

void WriteBuffer::WriteU16(uint16_t value) {
    EnsureCapacity(2);
    m_Data[m_WritePos++] = static_cast<uint8_t>(value & 0xFF);
    m_Data[m_WritePos++] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

void WriteBuffer::WriteU32(uint32_t value) {
    EnsureCapacity(4);
    m_Data[m_WritePos++] = static_cast<uint8_t>(value & 0xFF);
    m_Data[m_WritePos++] = static_cast<uint8_t>((value >> 8) & 0xFF);
    m_Data[m_WritePos++] = static_cast<uint8_t>((value >> 16) & 0xFF);
    m_Data[m_WritePos++] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

void WriteBuffer::WriteU64(uint64_t value) {
    EnsureCapacity(8);
    for (int i = 0; i < 8; ++i) {
        m_Data[m_WritePos++] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
    }
}

void WriteBuffer::WriteI8(int8_t value) {
    WriteU8(static_cast<uint8_t>(value));
}

void WriteBuffer::WriteI16(int16_t value) {
    WriteU16(static_cast<uint16_t>(value));
}

void WriteBuffer::WriteI32(int32_t value) {
    WriteU32(static_cast<uint32_t>(value));
}

void WriteBuffer::WriteI64(int64_t value) {
    WriteU64(static_cast<uint64_t>(value));
}

void WriteBuffer::WriteF32(float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(float));
    WriteU32(bits);
}

void WriteBuffer::WriteF64(double value) {
    uint64_t bits;
    std::memcpy(&bits, &value, sizeof(double));
    WriteU64(bits);
}

void WriteBuffer::WriteBool(bool value) {
    WriteU8(value ? 1 : 0);
}

void WriteBuffer::WriteString(const std::string& value) {
    if (value.size() > 65535) {
        throw std::runtime_error("String too long for serialization");
    }
    WriteU16(static_cast<uint16_t>(value.size()));
    WriteBytes(value.data(), value.size());
}

void WriteBuffer::WriteVec2(const Vec2& value) {
    WriteF32(value.x);
    WriteF32(value.y);
}

void WriteBuffer::WriteVec3(const Vec3& value) {
    WriteF32(value.x);
    WriteF32(value.y);
    WriteF32(value.z);
}

void WriteBuffer::WriteBytes(const void* data, size_t size) {
    EnsureCapacity(size);
    std::memcpy(&m_Data[m_WritePos], data, size);
    m_WritePos += size;
}

// ============================================================
// READ BUFFER IMPLEMENTATION
// ============================================================

ReadBuffer::ReadBuffer(const uint8_t* data, size_t size)
    : m_Data(data), m_Size(size), m_ReadPos(0) {
}

ReadBuffer::ReadBuffer(const std::vector<uint8_t>& data)
    : m_Data(data.data()), m_Size(data.size()), m_ReadPos(0) {
}

uint8_t ReadBuffer::ReadU8() {
    if (!HasData(1)) {
        throw std::runtime_error("Buffer underflow");
    }
    return m_Data[m_ReadPos++];
}

uint16_t ReadBuffer::ReadU16() {
    if (!HasData(2)) {
        throw std::runtime_error("Buffer underflow");
    }
    uint16_t value = m_Data[m_ReadPos++];
    value |= static_cast<uint16_t>(m_Data[m_ReadPos++]) << 8;
    return value;
}

uint32_t ReadBuffer::ReadU32() {
    if (!HasData(4)) {
        throw std::runtime_error("Buffer underflow");
    }
    uint32_t value = m_Data[m_ReadPos++];
    value |= static_cast<uint32_t>(m_Data[m_ReadPos++]) << 8;
    value |= static_cast<uint32_t>(m_Data[m_ReadPos++]) << 16;
    value |= static_cast<uint32_t>(m_Data[m_ReadPos++]) << 24;
    return value;
}

uint64_t ReadBuffer::ReadU64() {
    if (!HasData(8)) {
        throw std::runtime_error("Buffer underflow");
    }
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value |= static_cast<uint64_t>(m_Data[m_ReadPos++]) << (i * 8);
    }
    return value;
}

int8_t ReadBuffer::ReadI8() {
    return static_cast<int8_t>(ReadU8());
}

int16_t ReadBuffer::ReadI16() {
    return static_cast<int16_t>(ReadU16());
}

int32_t ReadBuffer::ReadI32() {
    return static_cast<int32_t>(ReadU32());
}

int64_t ReadBuffer::ReadI64() {
    return static_cast<int64_t>(ReadU64());
}

float ReadBuffer::ReadF32() {
    uint32_t bits = ReadU32();
    float value;
    std::memcpy(&value, &bits, sizeof(float));
    return value;
}

double ReadBuffer::ReadF64() {
    uint64_t bits = ReadU64();
    double value;
    std::memcpy(&value, &bits, sizeof(double));
    return value;
}

bool ReadBuffer::ReadBool() {
    return ReadU8() != 0;
}

std::string ReadBuffer::ReadString() {
    uint16_t length = ReadU16();
    if (!HasData(length)) {
        throw std::runtime_error("Buffer underflow reading string");
    }
    std::string value(reinterpret_cast<const char*>(&m_Data[m_ReadPos]), length);
    m_ReadPos += length;
    return value;
}

Vec2 ReadBuffer::ReadVec2() {
    Vec2 value;
    value.x = ReadF32();
    value.y = ReadF32();
    return value;
}

Vec3 ReadBuffer::ReadVec3() {
    Vec3 value;
    value.x = ReadF32();
    value.y = ReadF32();
    value.z = ReadF32();
    return value;
}

void ReadBuffer::ReadBytes(void* dest, size_t size) {
    if (!HasData(size)) {
        throw std::runtime_error("Buffer underflow reading bytes");
    }
    std::memcpy(dest, &m_Data[m_ReadPos], size);
    m_ReadPos += size;
}

} // namespace MMO
