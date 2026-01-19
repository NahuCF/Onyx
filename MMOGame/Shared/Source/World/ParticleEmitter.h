#pragma once

#include "WorldObject.h"

namespace MMO {

class ParticleEmitter : public WorldObject {
public:
    ParticleEmitter(uint64_t guid, const std::string& name = "")
        : WorldObject(guid, WorldObjectType::PARTICLE_EMITTER, name) {}

    // Effect reference
    void SetEffectId(uint32_t id) { m_EffectId = id; }
    uint32_t GetEffectId() const { return m_EffectId; }

    void SetEffectPath(const std::string& path) { m_EffectPath = path; }
    const std::string& GetEffectPath() const { return m_EffectPath; }

    // Playback settings
    void SetLooping(bool looping) { m_Looping = looping; }
    bool IsLooping() const { return m_Looping; }

    void SetAutoPlay(bool autoPlay) { m_AutoPlay = autoPlay; }
    bool IsAutoPlay() const { return m_AutoPlay; }

    void SetPlaybackSpeed(float speed) { m_PlaybackSpeed = speed; }
    float GetPlaybackSpeed() const { return m_PlaybackSpeed; }

    // Scale override
    void SetEffectScale(float scale) { m_EffectScale = scale; }
    float GetEffectScale() const { return m_EffectScale; }

    const char* GetTypeName() const override { return "Particle Emitter"; }

    std::unique_ptr<WorldObject> Clone(uint64_t newGuid) const override {
        auto copy = std::make_unique<ParticleEmitter>(newGuid, GetName() + " Copy");
        copy->SetPosition(GetPosition());
        copy->SetRotation(GetRotation());
        copy->SetScale(GetScale());
        copy->m_EffectId = m_EffectId;
        copy->m_EffectPath = m_EffectPath;
        copy->m_Looping = m_Looping;
        copy->m_AutoPlay = m_AutoPlay;
        copy->m_PlaybackSpeed = m_PlaybackSpeed;
        copy->m_EffectScale = m_EffectScale;
        return copy;
    }

private:
    uint32_t m_EffectId = 0;
    std::string m_EffectPath;

    bool m_Looping = true;
    bool m_AutoPlay = true;
    float m_PlaybackSpeed = 1.0f;
    float m_EffectScale = 1.0f;
};

} // namespace MMO
