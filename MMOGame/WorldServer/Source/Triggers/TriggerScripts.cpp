#include "TriggerScript.h"

#include "../../../Shared/Source/Scripting/ScriptRegistry.h"
#include "../Entity/Entity.h"
#include "../Map/MapDefines.h"
#include "../Map/MapInstance.h"
#include "../Scripting/IEntity.h"
#include "../Scripting/IMapContext.h"

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

	} // namespace

	void RegisterAllTriggerScripts()
	{
		auto& reg = ScriptRegistry<TriggerScript>::Instance();
		reg.Register<LogTriggerScript>();

		std::cout << "[Scripts] Registered " << reg.Size() << " TriggerScript(s):";
		reg.ForEach([](const ScriptObject& s) { std::cout << " " << s.GetName(); });
		std::cout << '\n';
	}

} // namespace MMO
