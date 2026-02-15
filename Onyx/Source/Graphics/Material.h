#pragma once

#include "Shader.h"
#include "Texture.h"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace Onyx {

class Material {
public:
    Material() = default;
    Material(Shader* shader) : m_Shader(shader) {}

    void SetShader(Shader* shader) { m_Shader = shader; }
    Shader* GetShader() const { return m_Shader; }

    void SetTexture(const std::string& name, Texture* texture) { m_Textures[name] = texture; }
    Texture* GetTexture(const std::string& name) const {
        auto it = m_Textures.find(name);
        return (it != m_Textures.end()) ? it->second : nullptr;
    }

    void SetFloat(const std::string& name, float value) { m_Floats[name] = value; }
    void SetVec3(const std::string& name, const glm::vec3& value) { m_Vec3s[name] = value; }
    void SetVec4(const std::string& name, const glm::vec4& value) { m_Vec4s[name] = value; }
    void SetMat4(const std::string& name, const glm::mat4& value) { m_Mat4s[name] = value; }

    void Bind() {
        if (!m_Shader) return;
        m_Shader->Bind();

        int textureUnit = 0;
        for (auto& [name, texture] : m_Textures) {
            if (texture) {
                texture->Bind(textureUnit);
                m_Shader->SetInt(name, textureUnit);
                textureUnit++;
            }
        }

        for (auto& [name, value] : m_Floats) {
            m_Shader->SetFloat(name, value);
        }
        for (auto& [name, value] : m_Vec3s) {
            m_Shader->SetVec3(name, value);
        }
        for (auto& [name, value] : m_Vec4s) {
            m_Shader->SetVec4(name, value.x, value.y, value.z, value.w);
        }
        for (auto& [name, value] : m_Mat4s) {
            m_Shader->SetMat4(name, value);
        }
    }

    void Unbind() {
        if (m_Shader) m_Shader->UnBind();
    }

private:
    Shader* m_Shader = nullptr;
    std::unordered_map<std::string, Texture*> m_Textures;
    std::unordered_map<std::string, float> m_Floats;
    std::unordered_map<std::string, glm::vec3> m_Vec3s;
    std::unordered_map<std::string, glm::vec4> m_Vec4s;
    std::unordered_map<std::string, glm::mat4> m_Mat4s;
};

} // namespace Onyx
