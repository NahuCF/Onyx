#include "TriggerScript.h"

#include "../../../Shared/Source/Scripting/ScriptRegistry.h"
#include "../Entity/Entity.h"
#include "../Map/MapDefines.h"
#include "../Map/MapInstance.h"
#include "../Scripting/IEntity.h"
#include "../Scripting/IMapContext.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

namespace MMO {

	namespace {

		void PrintLine(const char* verb, IEntity& entity, IMapContext& map,
					   const ServerTriggerVolume& trigger)
		{
			Vec2 ePos = entity.GetPosition();
			float eHeight = entity.GetHeight();
			const float dx = ePos.x - trigger.position.x;
			const float dy = ePos.y - trigger.position.y;
			const float dz = eHeight - trigger.positionZ;
			const float distXY = std::sqrt(dx * dx + dy * dy);

			std::cout << std::fixed << std::setprecision(2)
					  << "[Trigger] " << verb << " '" << trigger.guid << "'"
					  << " | map=" << map.GetMapName()
					  << " | t=" << map.GetTime() << "s"
					  << " | entity=" << entity.GetName() << " (" << entity.GetId() << ")"
					  << " at (" << ePos.x << ", " << ePos.y << ", " << eHeight << ")"
					  << " | volume center=(" << trigger.position.x << ", "
					  << trigger.position.y << ", " << trigger.positionZ << ")"
					  << " distXY=" << distXY;

			if (!trigger.scriptName.empty())
				std::cout << " | script=" << trigger.scriptName;
			if (trigger.eventId != 0)
				std::cout << " | eventId=" << trigger.eventId;
			std::cout << '\n';
		}

		// "log" — verbose enter/exit/stay logger.
		// Set ScriptName = "log" in the Inspector to enable for a volume.
		class LogTriggerScript : public TriggerScript
		{
		public:
			LogTriggerScript() : TriggerScript("log") {}

			void OnEnter(IMapContext& map, IEntity& entity, const ServerTriggerVolume& trigger) override
			{
				PrintLine("ENTER ", entity, map, trigger);
			}

			void OnExit(IMapContext& map, IEntity& entity, const ServerTriggerVolume& trigger) override
			{
				PrintLine("EXIT  ", entity, map, trigger);
			}

			void OnStay(IMapContext& map, IEntity& entity, const ServerTriggerVolume& trigger) override
			{
				PrintLine("STAY  ", entity, map, trigger);
			}
		};

		// "SpawnPack" — ambush trigger that spawns a fixed-size pack of
		// creatures around the volume's center on first entry. Backbone for
		// the tech-demo "walk into clearing → mobs appear" beat.
		//
		// Authoring (Inspector):
		//   ScriptName  = "SpawnPack"
		//   EventId     = creature_template id to spawn
		//   TriggerOnce = true  (otherwise the pack will re-spawn on every
		//                        movement-tick re-entry, almost always a bug)
		//
		// Behaviour: SPAWN_COUNT creatures placed at evenly-spaced points on a
		// circle of radius `spread` centred on the trigger. `spread` clamps to
		// 70 % of the volume's XY footprint so spawns stay near the trigger on
		// small volumes.
		//
		// Forward path: a per-trigger config (count, spread, alt templates)
		// schema lands once Phase 4 authoring UI is in. eventId-as-template-id
		// is the v1 contract.
		class SpawnPackTriggerScript : public TriggerScript
		{
		public:
			static constexpr uint32_t SPAWN_COUNT = 3;
			static constexpr float    SPREAD_MAX  = 1.5f;
			static constexpr float    TWO_PI      = 6.2831853071795864769f;

			SpawnPackTriggerScript() : TriggerScript("SpawnPack") {}

			void OnEnter(IMapContext& map, IEntity& entity,
						 const ServerTriggerVolume& trigger) override
			{
				const uint32_t templateId = trigger.eventId;
				if (templateId == 0)
				{
					std::cerr << "[SpawnPack] '" << trigger.guid
							  << "' EventId=0; nothing to spawn (set it to a creature_template id)\n";
					return;
				}

				if (!trigger.triggerOnce)
				{
					std::cerr << "[SpawnPack] '" << trigger.guid
							  << "' TriggerOnce=false; the pack will re-spawn on every entry "
							  << "(set TriggerOnce=true in the Inspector to fix)\n";
				}

				const float spread = std::min(SPREAD_MAX, trigger.BoundingRadiusXY() * 0.7f);

				for (uint32_t i = 0; i < SPAWN_COUNT; ++i)
				{
					const float angle = (static_cast<float>(i) / SPAWN_COUNT) * TWO_PI;
					const Vec2 pos(trigger.position.x + std::cos(angle) * spread,
								   trigger.position.y + std::sin(angle) * spread);

					if (!map.SummonCreature(templateId, pos, INVALID_ENTITY_ID))
					{
						std::cerr << "[SpawnPack] '" << trigger.guid
								  << "' failed at i=" << i
								  << " template_id=" << templateId
								  << " (unknown template? see CreatureTemplates registry)\n";
						return;
					}
				}

				std::cout << "[SpawnPack] '" << trigger.guid << "' spawned "
						  << SPAWN_COUNT << " of template_id=" << templateId
						  << " around (" << trigger.position.x << ", "
						  << trigger.position.y << ") triggered by '"
						  << entity.GetName() << "'\n";
			}
		};

	} // namespace

	void RegisterAllTriggerScripts()
	{
		auto& reg = ScriptRegistry<TriggerScript>::Instance();
		reg.Register<LogTriggerScript>();
		reg.Register<SpawnPackTriggerScript>();

		std::cout << "[Scripts] Registered " << reg.Size() << " TriggerScript(s):";
		reg.ForEach([](const ScriptObject& s) { std::cout << " " << s.GetName(); });
		std::cout << '\n';
	}

} // namespace MMO
