#pragma once

#include "ScriptObject.h"
#include <concepts>
#include <vector>

namespace MMO {

	// ============================================================
	// HOOK REGISTRY — non-owning pointers to subscribed scripts.
	// ScriptRegistry<T> owns the instances; HookRegistry<T> borrows
	// them to broadcast global events to all subscribers.
	//
	// Usage:
	//   HookRegistry<PlayerScript>::Instance().Dispatch<&PlayerScript::OnLogin>(entity);
	// ============================================================

	template<typename T>
	requires std::derived_from<T, ScriptObject>
	class HookRegistry
	{
	public:
		static HookRegistry& Instance()
		{
			static HookRegistry r;
			return r;
		}

		// Subscribe a script to receive global hook calls.
		// ScriptRegistry must outlive HookRegistry (both are Meyers singletons —
		// destruction order is unspecified, so use Clear() before shutdown).
		void Subscribe(T* script) { m_Subscribers.push_back(script); }

		void Clear() { m_Subscribers.clear(); }

		// Invoke Method on every subscriber.
		// Example: Dispatch<&PlayerScript::OnLogin>(*entity)
		template<auto Method, typename... Args>
		void Dispatch(Args&&... args)
		{
			for (T* s : m_Subscribers)
				(s->*Method)(std::forward<Args>(args)...);
		}

	private:
		HookRegistry() = default;
		std::vector<T*> m_Subscribers;
	};

} // namespace MMO
