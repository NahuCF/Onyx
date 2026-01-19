#pragma once

#include <vector>
#include <cstdint>
#include <stdexcept>

namespace Onyx {

// Supported vertex attribute types
enum class VertexAttributeType {
    Float,
    Float2,
    Float3,
    Float4,
    Int,
    Int2,
    Int3,
    Int4,
    UInt,
    Bool
};

// Get size in bytes for each attribute type
inline uint32_t GetAttributeTypeSize(VertexAttributeType type) {
    switch (type) {
        case VertexAttributeType::Float:  return 4;
        case VertexAttributeType::Float2: return 4 * 2;
        case VertexAttributeType::Float3: return 4 * 3;
        case VertexAttributeType::Float4: return 4 * 4;
        case VertexAttributeType::Int:    return 4;
        case VertexAttributeType::Int2:   return 4 * 2;
        case VertexAttributeType::Int3:   return 4 * 3;
        case VertexAttributeType::Int4:   return 4 * 4;
        case VertexAttributeType::UInt:   return 4;
        case VertexAttributeType::Bool:   return 1;
    }
    return 0;
}

// Get component count for each attribute type
inline uint32_t GetAttributeComponentCount(VertexAttributeType type) {
    switch (type) {
        case VertexAttributeType::Float:  return 1;
        case VertexAttributeType::Float2: return 2;
        case VertexAttributeType::Float3: return 3;
        case VertexAttributeType::Float4: return 4;
        case VertexAttributeType::Int:    return 1;
        case VertexAttributeType::Int2:   return 2;
        case VertexAttributeType::Int3:   return 3;
        case VertexAttributeType::Int4:   return 4;
        case VertexAttributeType::UInt:   return 1;
        case VertexAttributeType::Bool:   return 1;
    }
    return 0;
}

// Single vertex attribute description
struct VertexAttribute {
    VertexAttributeType type;
    uint32_t size;
    uint32_t offset;
    bool normalized;

    VertexAttribute(VertexAttributeType type, bool normalized = false)
        : type(type), size(GetAttributeTypeSize(type)), offset(0), normalized(normalized) {}
};

// Describes the layout of vertex data
class VertexLayout {
public:
    VertexLayout() = default;

    VertexLayout(std::initializer_list<VertexAttribute> attributes)
        : m_Attributes(attributes) {
        CalculateOffsetsAndStride();
    }

    // Add attribute using type
    void Push(VertexAttributeType type, bool normalized = false) {
        m_Attributes.push_back(VertexAttribute(type, normalized));
        CalculateOffsetsAndStride();
    }

    // Convenience methods
    void PushFloat(uint32_t count, bool normalized = false) {
        switch (count) {
            case 1: Push(VertexAttributeType::Float, normalized); break;
            case 2: Push(VertexAttributeType::Float2, normalized); break;
            case 3: Push(VertexAttributeType::Float3, normalized); break;
            case 4: Push(VertexAttributeType::Float4, normalized); break;
        }
    }

    void PushInt(uint32_t count) {
        switch (count) {
            case 1: Push(VertexAttributeType::Int); break;
            case 2: Push(VertexAttributeType::Int2); break;
            case 3: Push(VertexAttributeType::Int3); break;
            case 4: Push(VertexAttributeType::Int4); break;
        }
    }

    uint32_t GetStride() const { return m_Stride; }
    const std::vector<VertexAttribute>& GetAttributes() const { return m_Attributes; }
    size_t GetAttributeCount() const { return m_Attributes.size(); }

private:
    void CalculateOffsetsAndStride() {
        uint32_t offset = 0;
        m_Stride = 0;
        for (auto& attr : m_Attributes) {
            attr.offset = offset;
            offset += attr.size;
            m_Stride += attr.size;
        }
    }

private:
    std::vector<VertexAttribute> m_Attributes;
    uint32_t m_Stride = 0;
};

}
