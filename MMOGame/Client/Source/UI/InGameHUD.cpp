#include "InGameHUD.h"

#include "../GameClient.h"

#include "Source/Core/Application.h"
#include "Source/Graphics/WorldUIAnchorSystem.h"
#include "Source/UI/Color.h"
#include "Source/UI/Layout.h"
#include "Source/UI/Manager.h"
#include "Source/UI/ScreenStack.h"
#include "Source/UI/Text/FontAtlas.h"
#include "Source/UI/Widget.h"
#include "Source/UI/WidgetTree.h"
#include "Source/UI/Widgets/FloatingText.h"
#include "Source/UI/Widgets/Nameplate.h"
#include "Source/UI/Widgets/ProgressBar.h"

#include <algorithm>
#include <sstream>
#include <utility>

namespace MMO {

	namespace {

		constexpr float HP_BAR_WIDTH         = 240.0f;
		constexpr float HP_BAR_HEIGHT        = 24.0f;
		constexpr float HP_BAR_EDGE_PAD      = 16.0f;
		constexpr float OVERHEAD_Z_OFFSET    = 1.6f;   // world units above feet
		constexpr float FLOAT_LIFETIME       = 1.1f;
		constexpr float FLOAT_TEXT_PX        = 18.0f;

		float SafeFraction(int32_t current, int32_t max)
		{
			if (max <= 0)
				return 0.0f;
			const float f = static_cast<float>(current) / static_cast<float>(max);
			return std::clamp(f, 0.0f, 1.0f);
		}

	} // namespace

	InGameHUD::InGameHUD(Onyx::Application& app, GameClient& client)
		: m_App(app), m_Client(client)
	{
		m_Manager      = &m_App.GetUI();
		m_AnchorSystem = &m_App.GetWorldUIAnchorSystem();

		BuildTree();

		m_Client.SetDamageHandler(
			[this](EntityId src, EntityId tgt, int32_t dmg, Vec2 pos) {
				HandleDamage(src, tgt, dmg, pos);
			});
	}

	InGameHUD::~InGameHUD()
	{
		// Clear the handler first so a tail event can't reach a dead `this`.
		m_Client.SetDamageHandler({});
		if (IsAttached())
			DetachFromManager();
	}

	void InGameHUD::BuildTree()
	{
		using namespace Onyx::UI;

		m_Tree         = std::make_unique<WidgetTree>();
		m_PlayerHpBar  = nullptr;
		m_TargetHpBar  = nullptr;
		m_ActiveFloats.clear();
		m_Nameplates.clear();

		auto root = std::make_unique<Widget>();
		root->SetContainerMode(ContainerMode::Free);
		root->FillSize();

		// Player HP — anchored TopLeft.
		{
			auto bar = std::make_unique<ProgressBar>();
			bar->SetRange(0.0f, 1.0f);
			bar->SetCurrent(1.0f);
			bar->SetFillColor(Color::FromRGBA8(0x4CAF50FF));  // green
			bar->SetBorderColor(Color::FromRGBA8(0x000000C0));
			bar->FixedSize(HP_BAR_WIDTH, HP_BAR_HEIGHT)
				.Anchor(AnchorEdge::TopLeft, {HP_BAR_EDGE_PAD, HP_BAR_EDGE_PAD});
			if (m_Manager->DefaultFontLoaded())
				bar->SetLabel(&m_Manager->GetDefaultFont(), "", 13.0f, Color::White());

			m_PlayerHpBar = bar.get();
			root->AddChild(std::move(bar));
		}

		// Target HP — anchored TopRight, hidden until LocalPlayer.targetId is set.
		{
			auto bar = std::make_unique<ProgressBar>();
			bar->SetRange(0.0f, 1.0f);
			bar->SetCurrent(1.0f);
			bar->SetFillColor(Color::FromRGBA8(0xC0382EFF));  // red
			bar->SetBorderColor(Color::FromRGBA8(0x000000C0));
			bar->FixedSize(HP_BAR_WIDTH, HP_BAR_HEIGHT)
				.Anchor(AnchorEdge::TopRight, {-HP_BAR_EDGE_PAD, HP_BAR_EDGE_PAD});
			if (m_Manager->DefaultFontLoaded())
				bar->SetLabel(&m_Manager->GetDefaultFont(), "", 13.0f, Color::White());
			bar->SetVisible(false);

			m_TargetHpBar = bar.get();
			root->AddChild(std::move(bar));
		}

		m_Tree->SetRoot(std::move(root));
	}

	void InGameHUD::AttachToManager()
	{
		if (m_AttachedHandle || !m_Tree)
			return;
		m_AttachedHandle =
			m_Manager->PushOverlay(std::move(m_Tree), Onyx::UI::ScreenLayer::HUD);
	}

	void InGameHUD::DetachFromManager()
	{
		if (!m_AttachedHandle)
			return;
		m_Manager->PopOverlay(m_AttachedHandle);
		m_AttachedHandle = nullptr;
		// Pop destroys the tree; rebuild a fresh one so a later Attach works.
		BuildTree();
	}

	void InGameHUD::Update(float /*dt*/)
	{
		if (!m_AttachedHandle)
			return;
		DriveBars();
		SyncNameplates();
		GarbageCollectFloats();
	}

	void InGameHUD::DriveBars()
	{
		const auto& player = m_Client.GetLocalPlayer();

		if (m_PlayerHpBar)
		{
			m_PlayerHpBar->SetCurrent(SafeFraction(player.health, player.maxHealth));
			if (m_Manager->DefaultFontLoaded())
			{
				std::ostringstream s;
				s << player.health << " / " << player.maxHealth;
				m_PlayerHpBar->SetLabelText(s.str());
			}
		}

		if (!m_TargetHpBar)
			return;

		const EntityId tid = player.targetId;
		if (tid == INVALID_ENTITY_ID)
		{
			m_TargetHpBar->SetVisible(false);
			return;
		}

		const auto& ents = m_Client.GetEntities();
		auto it = ents.find(tid);
		if (it == ents.end())
		{
			// Target id set but entity not yet replicated — render the frame
			// hidden rather than stale. Will flip on once the entity lands.
			m_TargetHpBar->SetVisible(false);
			return;
		}

		const RemoteEntity& t = it->second;
		m_TargetHpBar->SetVisible(true);
		m_TargetHpBar->SetCurrent(SafeFraction(t.health, t.maxHealth));
		if (m_Manager->DefaultFontLoaded())
		{
			std::ostringstream s;
			s << t.name << "  " << t.health << " / " << t.maxHealth;
			m_TargetHpBar->SetLabelText(s.str());
		}
	}

	void InGameHUD::HandleDamage(EntityId /*source*/, EntityId target,
								 int32_t amount, Vec2 groundPos)
	{
		if (!m_AttachedHandle || !m_AnchorSystem)
			return;
		if (!m_Manager->DefaultFontLoaded() || amount == 0)
			return;

		// Pull the target's vertical height so the number spawns over its head.
		float zHeight = 0.0f;
		if (target != INVALID_ENTITY_ID)
		{
			const auto& localPlayer = m_Client.GetLocalPlayer();
			if (localPlayer.entityId == target)
			{
				zHeight = localPlayer.height;
			}
			else
			{
				const auto& ents = m_Client.GetEntities();
				auto it = ents.find(target);
				if (it != ents.end())
					zHeight = it->second.height;
			}
		}

		// Negative `amount` means a heal in the wire protocol (DAMAGE event
		// piggy-backs on HEAL via sign). Tint accordingly. The user-facing
		// string is always positive magnitude.
		const bool isHeal = amount < 0;
		const Onyx::UI::Color color = isHeal
			? Onyx::UI::Color::FromRGBA8(0x80FF80FF)  // green
			: Onyx::UI::Color::FromRGBA8(0xFFB347FF); // amber
		std::string text = std::to_string(isHeal ? -amount : amount);

		auto ft = std::make_unique<Onyx::UI::FloatingText>(
			&m_Manager->GetDefaultFont(), std::move(text), color, FLOAT_TEXT_PX);
		ft->SetAnchorSystem(m_AnchorSystem);
		ft->SetWorldPosition({groundPos.x, groundPos.y, zHeight});
		ft->SetWorldOffset({0.0f, 0.0f, OVERHEAD_Z_OFFSET});
		ft->SetEntityId(static_cast<uint64_t>(target));
		ft->SetLifetime(FLOAT_LIFETIME);

		auto* raw = ft.get();
		if (auto* root = m_AttachedHandle->GetRoot())
		{
			root->AddChild(std::move(ft));
			m_ActiveFloats.push_back(raw);
		}
	}

	void InGameHUD::SyncNameplates()
	{
		if (!m_AnchorSystem || !m_AttachedHandle)
			return;

		auto* root = m_AttachedHandle->GetRoot();
		if (!root)
			return;

		const Onyx::UI::FontAtlas* font =
			m_Manager->DefaultFontLoaded() ? &m_Manager->GetDefaultFont() : nullptr;

		const auto& entities = m_Client.GetEntities();

		// Mount new entities + refresh existing per-frame state.
		for (const auto& [id, ent] : entities)
		{
			Onyx::UI::Nameplate* plate = nullptr;
			auto it = m_Nameplates.find(id);
			if (it == m_Nameplates.end())
			{
				auto fresh = std::make_unique<Onyx::UI::Nameplate>(font, 13.0f);
				fresh->SetAnchorSystem(m_AnchorSystem);
				fresh->SetEntityId(static_cast<uint64_t>(id));
				fresh->SetWorldOffset({0.0f, 0.0f, OVERHEAD_Z_OFFSET});
				plate = fresh.get();
				root->AddChild(std::move(fresh));
				m_Nameplates.emplace(id, plate);
			}
			else
			{
				plate = it->second;
			}

			plate->SetName(ent.name);
			plate->SetWorldPosition({ent.position.x, ent.position.y, ent.height});
			plate->SetHealthFraction(SafeFraction(ent.health, ent.maxHealth));
			plate->SetShowHealth(ent.maxHealth > 0);
		}

		// Unmount entities that have disappeared from the grid.
		for (auto it = m_Nameplates.begin(); it != m_Nameplates.end(); )
		{
			if (entities.find(it->first) == entities.end())
			{
				root->RemoveChild(it->second);
				it = m_Nameplates.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void InGameHUD::GarbageCollectFloats()
	{
		if (m_ActiveFloats.empty() || !m_AttachedHandle)
			return;

		auto* root = m_AttachedHandle->GetRoot();
		if (!root)
		{
			m_ActiveFloats.clear();
			return;
		}

		auto newEnd = std::remove_if(
			m_ActiveFloats.begin(), m_ActiveFloats.end(),
			[root](Onyx::UI::FloatingText* w) {
				if (!w || w->IsDead())
				{
					if (w)
						root->RemoveChild(w);
					return true;
				}
				return false;
			});
		m_ActiveFloats.erase(newEnd, m_ActiveFloats.end());
	}

} // namespace MMO
