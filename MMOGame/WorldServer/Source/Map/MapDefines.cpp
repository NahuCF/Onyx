#include "MapDefines.h"
#include "../AI/CreatureTemplate.h"

namespace MMO {

// ============================================================
// MOB SPAWN POINT IMPLEMENTATIONS
// ============================================================

float MobSpawnPoint::GetRespawnTime(const CreatureTemplate* tmpl) const {
    if (respawnTimeOverride > 0) return respawnTimeOverride;
    return tmpl ? tmpl->GetRespawnTime() : GetDefaultRespawnTime(CreatureRank::NORMAL);
}

float MobSpawnPoint::GetCorpseDecayTime(const CreatureTemplate* tmpl) const {
    if (corpseDecayTimeOverride > 0) return corpseDecayTimeOverride;
    return tmpl ? tmpl->GetCorpseDecayTime() : GetDefaultCorpseDecayTime(CreatureRank::NORMAL);
}

} // namespace MMO
