#pragma once

#include "ScriptObject.h"
#include <concepts>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace MMO {

	// ============================================================
	// SCRIPT REGISTRY — owns unique_ptr<T> keyed by script name.
	// One instantiation per concrete script type (TriggerScript,
	// CreatureScript, etc.). Meyers singleton; thread-safe init.
	// ============================================================

	template<typename T>
	requires std::derived_from<T, ScriptObject>
	class ScriptRegistry
	{
	public:
		static ScriptRegistry& Instance()
		{
			static ScriptRegistry r;
			return r;
		}

		// Construct U (must derive from T) and insert it.
		// Returns raw pointer to the registered instance.
		template<typename U>
		requires std::derived_from<U, T>
		T* Register()
		{
			auto script = std::make_unique<U>();
			T* raw = script.get();
			std::string name(script->GetName());
			m_Scripts[name] = std::move(script);
			return raw;
		}

		// O(1) hash lookup by name. Returns nullptr if not found.
		T* Get(std::string_view name) const
		{
			auto it = m_Scripts.find(std::string(name));
			return it == m_Scripts.end() ? nullptr : it->second.get();
		}

		size_t Size() const { return m_Scripts.size(); }

		void ForEach(auto&& fn) const
		{
			for (auto& [_, s] : m_Scripts)
				fn(*s);
		}

		void Clear() { m_Scripts.clear(); }

	private:
		ScriptRegistry() = default;
		std::unordered_map<std::string, std::unique_ptr<T>> m_Scripts;
	};

} // namespace MMO
