#include "../../../Shared/Source/Scripting/ScriptRegistry.h"
#include "SpellScript.h"
#include <iostream>

namespace MMO {

	void RegisterAllSpellScripts()
	{
		auto& reg = ScriptRegistry<SpellScript>::Instance();
		std::cout << "[Scripts] Registered " << reg.Size() << " SpellScript(s)\n";
	}

} // namespace MMO
