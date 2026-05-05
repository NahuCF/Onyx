#include "../../../Shared/Source/Scripting/ScriptRegistry.h"
#include "QuestScript.h"
#include <iostream>

namespace MMO {

	void RegisterAllQuestScripts()
	{
		auto& reg = ScriptRegistry<QuestScript>::Instance();
		std::cout << "[Scripts] Registered " << reg.Size() << " QuestScript(s)\n";
	}

} // namespace MMO
