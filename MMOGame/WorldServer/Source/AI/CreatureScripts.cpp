#include "../../../Shared/Source/Scripting/ScriptRegistry.h"
#include "CreatureScript.h"
#include "../Scripts/ShadowLordAI.h"
#include <iostream>

namespace MMO {

	void RegisterAllCreatureScripts()
	{
		auto& reg = ScriptRegistry<CreatureScript>::Instance();

		reg.Register<ShadowLordScript>();

		std::cout << "[Scripts] Registered " << reg.Size() << " CreatureScript(s):";
		reg.ForEach([](const ScriptObject& s) { std::cout << " " << s.GetName(); });
		std::cout << '\n';
	}

} // namespace MMO
