#include "ScriptRegistry.h"
#include "ScriptedAI.h"
#include "CreatureTemplate.h"

namespace MMO {

std::unique_ptr<ScriptedAI> ScriptRegistry::CreateAI(Entity* owner, const CreatureTemplate* tmpl) {
    // Check if creature has a custom script
    if (!tmpl->scriptName.empty()) {
        auto it = m_Scripts.find(tmpl->scriptName);
        if (it != m_Scripts.end()) {
            return it->second(owner, tmpl);
        }
    }
    // No custom script - return default ScriptedAI
    return std::make_unique<ScriptedAI>(owner, tmpl);
}

} // namespace MMO
