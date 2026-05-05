#include "../../../Shared/Source/Scripting/ScriptRegistry.h"
#include "InstanceScript.h"
#include <iostream>

namespace MMO {

	void RegisterAllInstanceScripts()
	{
		auto& reg = ScriptRegistry<InstanceScript>::Instance();

		// Register instance scripts here when they exist:
		// reg.Register<DungeonOfDoomScript>();

		std::cout << "[Scripts] Registered " << reg.Size() << " InstanceScript(s)\n";
	}

} // namespace MMO
