#include "TriggerScript.h"

namespace MMO {

	TriggerScriptRegistry& TriggerScriptRegistry::Instance()
	{
		static TriggerScriptRegistry instance;
		return instance;
	}

	void TriggerScriptRegistry::Register(const std::string& name, std::unique_ptr<TriggerScript> script)
	{
		m_Scripts[name] = std::move(script);
	}

	TriggerScript* TriggerScriptRegistry::Get(const std::string& name) const
	{
		auto it = m_Scripts.find(name);
		return it == m_Scripts.end() ? nullptr : it->second.get();
	}

} // namespace MMO
