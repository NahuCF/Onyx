#pragma once

#include "../../../Shared/Source/Types/Types.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Onyx
{
	class Application;
	class WorldUIAnchorSystem;
}

namespace Onyx::UI
{
	class Manager;
	class WidgetTree;
	class Widget;
	class ProgressBar;
	class FloatingText;
	class Nameplate;
}

namespace MMO {

	class GameClient;

	// Live in-game HUD: player HP bar (top-left), target HP bar (top-right,
	// only when GameClient::LocalPlayer.targetId is set), and floating damage
	// numbers spawned at the hit world position.
	//
	// Pumps state imperatively from GameClient each frame — M7 reactive
	// bindings haven't shipped, and the design doc explicitly permits
	// imperative wiring for the demo. Hot path is O(1) for the bars +
	// O(active_floats) for damage-number GC; tested with ~10 concurrent.
	//
	// Lifecycle:
	//   InGameHUD hud(app, gameClient);  // builds offline tree
	//   hud.AttachToManager();           // pushes onto Manager (ScreenLayer::HUD)
	//   // ... each frame, after GameClient::Update ...
	//   hud.Update(dt);                  // bars + damage-number GC
	//   // ... on world exit / shutdown ...
	//   hud.DetachFromManager();         // pops cleanly
	//
	// Damage numbers: wired through GameClient::SetDamageHandler in the ctor;
	// the dtor clears the handler so the HUD can be torn down while the
	// client lives on.
	class InGameHUD
	{
	public:
		InGameHUD(Onyx::Application& app, GameClient& client);
		~InGameHUD();

		InGameHUD(const InGameHUD&) = delete;
		InGameHUD& operator=(const InGameHUD&) = delete;

		void AttachToManager();
		void DetachFromManager();
		bool IsAttached() const { return m_AttachedHandle != nullptr; }

		void Update(float dt);

	private:
		void BuildTree();
		void DriveBars();
		void SyncNameplates();
		void GarbageCollectFloats();

		// Bound to GameClient::SetDamageHandler.
		void HandleDamage(EntityId source, EntityId target, int32_t amount, Vec2 groundPos);

		Onyx::Application&         m_App;
		GameClient&                m_Client;
		Onyx::UI::Manager*         m_Manager      = nullptr;
		Onyx::WorldUIAnchorSystem* m_AnchorSystem = nullptr;

		std::unique_ptr<Onyx::UI::WidgetTree> m_Tree;
		Onyx::UI::WidgetTree*                 m_AttachedHandle = nullptr;

		// Owned by m_Tree's root; raw pointers for per-frame access.
		Onyx::UI::ProgressBar* m_PlayerHpBar = nullptr;
		Onyx::UI::ProgressBar* m_TargetHpBar = nullptr;

		// Live floating numbers; raw pointers into the attached tree.
		// Pruned when FloatingText::IsDead().
		std::vector<Onyx::UI::FloatingText*> m_ActiveFloats;

		// Per-RemoteEntity nameplates. Cache mounted on first sight of the
		// entity, unmounted when it disappears from GameClient::GetEntities.
		// Raw pointers into the attached tree.
		std::unordered_map<EntityId, Onyx::UI::Nameplate*> m_Nameplates;
	};

} // namespace MMO
