#pragma once

#include "WorldObject.h"

namespace MMO {

enum class LightType : uint8_t {
    POINT = 0,
    SPOT,
    DIRECTIONAL,
};

class Light : public WorldObject {
public:
    Light(uint64_t guid, LightType lightType = LightType::POINT, const std::string& name = "")
        : WorldObject(guid, WorldObjectType::LIGHT, name)
        , m_LightType(lightType) {}

    // Light type
    LightType GetLightType() const { return m_LightType; }
    void SetLightType(LightType type) { m_LightType = type; }

    // Common properties
    void SetColor(const glm::vec3& color) { m_Color = color; }
    const glm::vec3& GetColor() const { return m_Color; }

    void SetIntensity(float intensity) { m_Intensity = intensity; }
    float GetIntensity() const { return m_Intensity; }

    // Point/Spot light properties
    void SetRadius(float radius) { m_Radius = radius; }
    float GetRadius() const { return m_Radius; }

    void SetFalloff(float falloff) { m_Falloff = falloff; }
    float GetFalloff() const { return m_Falloff; }

    // Spot light properties
    void SetInnerAngle(float degrees) { m_InnerAngle = degrees; }
    float GetInnerAngle() const { return m_InnerAngle; }

    void SetOuterAngle(float degrees) { m_OuterAngle = degrees; }
    float GetOuterAngle() const { return m_OuterAngle; }

    // Shadow properties
    void SetCastsShadows(bool casts) { m_CastsShadows = casts; }
    bool CastsShadows() const { return m_CastsShadows; }

    void SetShadowBias(float bias) { m_ShadowBias = bias; }
    float GetShadowBias() const { return m_ShadowBias; }

    // Baking
    void SetBakedOnly(bool bakedOnly) { m_BakedOnly = bakedOnly; }
    bool IsBakedOnly() const { return m_BakedOnly; }

    const char* GetTypeName() const override {
        switch (m_LightType) {
            case LightType::POINT: return "Point Light";
            case LightType::SPOT: return "Spot Light";
            case LightType::DIRECTIONAL: return "Directional Light";
            default: return "Light";
        }
    }

    std::unique_ptr<WorldObject> Clone(uint64_t newGuid) const override {
        auto copy = std::make_unique<Light>(newGuid, m_LightType, GetName() + " Copy");
        copy->SetPosition(GetPosition());
        copy->SetRotation(GetRotation());
        copy->SetScale(GetScale());
        copy->m_Color = m_Color;
        copy->m_Intensity = m_Intensity;
        copy->m_Radius = m_Radius;
        copy->m_Falloff = m_Falloff;
        copy->m_InnerAngle = m_InnerAngle;
        copy->m_OuterAngle = m_OuterAngle;
        copy->m_CastsShadows = m_CastsShadows;
        copy->m_ShadowBias = m_ShadowBias;
        copy->m_BakedOnly = m_BakedOnly;
        return copy;
    }

private:
    LightType m_LightType = LightType::POINT;

    // Common
    glm::vec3 m_Color = glm::vec3(1.0f);
    float m_Intensity = 1.0f;

    // Point/Spot
    float m_Radius = 10.0f;
    float m_Falloff = 1.0f;  // Attenuation curve

    // Spot
    float m_InnerAngle = 30.0f;  // Degrees
    float m_OuterAngle = 45.0f;  // Degrees

    // Shadows
    bool m_CastsShadows = false;
    float m_ShadowBias = 0.005f;

    // Baking
    bool m_BakedOnly = false;  // If true, only affects lightmaps, not runtime
};

} // namespace MMO
