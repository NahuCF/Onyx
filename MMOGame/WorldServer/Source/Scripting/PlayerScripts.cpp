#include "../../../Shared/Source/Scripting/ScriptRegistry.h"
#include "../../../Shared/Source/Scripting/HookRegistry.h"
#include "PlayerScript.h"
#include <iostream>

namespace MMO {

	void RegisterAllPlayerScripts()
	{
		auto& reg = ScriptRegistry<PlayerScript>::Instance();

		// Example registration pattern (add concrete scripts here):
		// auto* s = reg.Register<WelcomeScript>();
		// HookRegistry<PlayerScript>::Instance().Subscribe(s);

		std::cout << "[Scripts] Registered " << reg.Size() << " PlayerScript(s)\n";
	}

} // namespace MMO
