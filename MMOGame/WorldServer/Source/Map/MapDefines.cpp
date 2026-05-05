#include "MapDefines.h"
#include "../AI/CreatureTemplate.h"
#include <algorithm>
#include <cmath>

namespace MMO {

	// ============================================================
	// MOB SPAWN POINT IMPLEMENTATIONS
	// ============================================================

	float MobSpawnPoint::GetRespawnTime(const CreatureTemplate* tmpl) const
	{
		if (respawnTimeOverride > 0)
			return respawnTimeOverride;
		return tmpl ? tmpl->GetRespawnTime() : GetDefaultRespawnTime(CreatureRank::NORMAL);
	}

	float MobSpawnPoint::GetCorpseDecayTime(const CreatureTemplate* tmpl) const
	{
		return tmpl ? tmpl->GetCorpseDecayTime() : GetDefaultCorpseDecayTime(CreatureRank::NORMAL);
	}

	// ============================================================
	// SERVER TRIGGER VOLUME GEOMETRY
	// ============================================================

	bool ServerTriggerVolume::ContainsXY(Vec2 point) const
	{
		const float dx = point.x - position.x;
		const float dy = point.y - position.y;

		switch (shape)
		{
		case TriggerShapeKind::SPHERE:
		case TriggerShapeKind::CAPSULE:
			return (dx * dx + dy * dy) <= (radius * radius);

		case TriggerShapeKind::BOX:
		default:
		{
			const float c = std::cos(-orientation);
			const float s = std::sin(-orientation);
			const float lx = dx * c - dy * s;
			const float ly = dx * s + dy * c;
			return std::abs(lx) <= halfExtentX && std::abs(ly) <= halfExtentY;
		}
		}
	}

	bool ServerTriggerVolume::Contains(Vec2 ground, float vertical) const
	{
		if (!ContainsXY(ground))
			return false;

		const float dz = vertical - positionZ;
		switch (shape)
		{
		case TriggerShapeKind::SPHERE:
		{
			const float dx = ground.x - position.x;
			const float dy = ground.y - position.y;
			return (dx * dx + dy * dy + dz * dz) <= (radius * radius);
		}
		case TriggerShapeKind::CAPSULE:
		case TriggerShapeKind::BOX:
		default:
			return std::abs(dz) <= halfExtentZ;
		}
	}

	float ServerTriggerVolume::BoundingRadiusXY() const
	{
		switch (shape)
		{
		case TriggerShapeKind::SPHERE:
		case TriggerShapeKind::CAPSULE:
			return radius;
		case TriggerShapeKind::BOX:
		default:
			return std::sqrt(halfExtentX * halfExtentX + halfExtentY * halfExtentY);
		}
	}

} // namespace MMO
