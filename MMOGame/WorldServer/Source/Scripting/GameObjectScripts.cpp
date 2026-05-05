#include "../../../Shared/Source/Scripting/ScriptRegistry.h"
#include "GameObjectScript.h"
#include <iostream>

namespace MMO {

	void RegisterAllGameObjectScripts()
	{
		auto& reg = ScriptRegistry<GameObjectScript>::Instance();
		std::cout << "[Scripts] Registered " << reg.Size() << " GameObjectScript(s)\n";
	}

} // namespace MMO
