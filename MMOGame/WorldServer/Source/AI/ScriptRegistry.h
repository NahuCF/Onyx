#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace MMO {

class Entity;
class ScriptedAI;
struct CreatureTemplate;

// ============================================================
// SCRIPT REGISTRY - Maps creature IDs to custom AI scripts
// ============================================================

class ScriptRegistry {
public:
    using ScriptFactory = std::function<std::unique_ptr<ScriptedAI>(Entity*, const CreatureTemplate*)>;

    static ScriptRegistry& Instance() {
        static ScriptRegistry instance;
        return instance;
    }

    void RegisterScript(const std::string& scriptName, ScriptFactory factory) {
        m_Scripts[scriptName] = std::move(factory);
    }

    std::unique_ptr<ScriptedAI> CreateAI(Entity* owner, const CreatureTemplate* tmpl);

private:
    ScriptRegistry() = default;
    std::unordered_map<std::string, ScriptFactory> m_Scripts;
};

// ============================================================
// SCRIPT REGISTRATION MACRO
// ============================================================

#define REGISTER_CREATURE_SCRIPT(scriptName, AIClass) \
    namespace { \
        static bool _registered_##AIClass = []() { \
            ScriptRegistry::Instance().RegisterScript(scriptName, \
                [](Entity* owner, const CreatureTemplate* tmpl) { \
                    return std::make_unique<AIClass>(owner, tmpl); \
                }); \
            return true; \
        }(); \
    }

} // namespace MMO
