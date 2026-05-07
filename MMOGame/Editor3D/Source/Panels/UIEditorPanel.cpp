#include "UIEditorPanel.h"

#include <Core/Application.h>
#include <UI/Color.h>
#include <UI/Layout.h>
#include <UI/Manager.h>
#include <UI/Text/FontAtlas.h>
#include <UI/Widgets/Button.h>
#include <UI/Widgets/DebugRect.h>
#include <UI/Widgets/Label.h>
#include <UI/Widgets/TextInput.h>

#include <GL/glew.h>
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>

namespace MMO {

	using Onyx::UI::AnchorEdge;
	using Onyx::UI::Button;
	using Onyx::UI::Color;
	using Onyx::UI::ContainerMode;
	using Onyx::UI::DebugRect;
	using Onyx::UI::FontAtlas;
	using Onyx::UI::InputEvent;
	using Onyx::UI::InputEventType;
	using Onyx::UI::Label;
	using Onyx::UI::LayoutSpec;
	using Onyx::UI::ModifierFlag;
	using Onyx::UI::ModifierFlags;
	using Onyx::UI::MouseButton;
	using Onyx::UI::Rect2D;
	using Onyx::UI::SizeMode;
	using Onyx::UI::TextInput;
	using Onyx::UI::UIRenderer;
	using Onyx::UI::Widget;
	using Onyx::UI::WidgetTree;

	namespace {

		// Color helpers ------------------------------------------------------

		inline Color C(uint32_t rgba) { return Color::FromRGBA8(rgba); }

		inline ImVec4 ToImVec4(const Color& c) { return ImVec4(c.r, c.g, c.b, c.a); }

		// Stage-builder helpers ---------------------------------------------

		std::unique_ptr<DebugRect> MakeRect(uint32_t rgba, const char* id, bool interactive = false)
		{
			auto r = std::make_unique<DebugRect>(C(rgba), id);
			r->SetId(id);
			r->SetInteractive(interactive);
			return r;
		}

		std::unique_ptr<Label> MakeText(const FontAtlas& font, const char* text, float pxSize,
										uint32_t rgba,
										UIRenderer::TextAlignH align = UIRenderer::TextAlignH::Left)
		{
			auto l = std::make_unique<Label>(font, text, pxSize, C(rgba));
			l->SetAlign(align);
			return l;
		}

		// Anchored push: sets size + anchor, then transfers ownership to parent.
		void PushAnchored(Widget* parent, std::unique_ptr<Widget> child,
						  AnchorEdge anchor, glm::vec2 offset, glm::vec2 size)
		{
			child->FixedSize(size.x, size.y);
			child->Anchor(anchor, offset);
			parent->AddChild(std::move(child));
		}

		void PushFill(Widget* parent, std::unique_ptr<Widget> child)
		{
			child->FillSize();
			parent->AddChild(std::move(child));
		}

	} // namespace

	// =========================================================================
	// ctor / OnInit
	// =========================================================================

	UIEditorPanel::UIEditorPanel() = default;
	UIEditorPanel::~UIEditorPanel() = default;

	void UIEditorPanel::OnInit()
	{
		m_Renderer    = std::make_unique<UIRenderer>();
		m_Tree        = std::make_unique<WidgetTree>();
		m_Framebuffer = std::make_unique<Onyx::Framebuffer>();

		RegisterStages();
		RegisterResolutions();
		RegisterMockProfiles();
		RegisterThemes();
		RegisterLocales();

		// Default to In-Game HUD — most visually informative.
		for (size_t i = 0; i < m_Stages.size(); ++i)
		{
			if (m_Stages[i].name == "In-Game HUD")
			{
				m_CurrentStage = static_cast<int>(i);
				break;
			}
		}
	}

	// =========================================================================
	// Registration tables
	// =========================================================================

	void UIEditorPanel::RegisterStages()
	{
		m_Stages = {
			{ "Login",                  "Auth screen, server picker.",                    [this] { return BuildLoginStage(); } },
			{ "Character Select",       "Roster, create / delete buttons.",               [this] { return BuildCharacterSelectStage(); } },
			{ "Loading",                "World transition; progress bar.",                [this] { return BuildLoadingStage(); } },
			{ "In-Game HUD",            "Action bars, frames, chat, minimap.",            [this] { return BuildInGameHudStage(); } },
			{ "In-Game + Inventory",    "HUD with inventory modal pushed.",               [this] { return BuildInGameInventoryStage(); } },
			{ "Combat - Low HP",        "HUD under combat, low HP, multiple buffs.",      [this] { return BuildCombatLowHpStage(); } },
			{ "Death Screen",           "Death overlay, respawn options.",                [this] { return BuildDeathScreenStage(); } },
		};
	}

	void UIEditorPanel::RegisterResolutions()
	{
		m_Resolutions = {
			{ "1920x1080  (16:9)",       1920, 1080, 0  },
			{ "2560x1440  (16:9 1440p)", 2560, 1440, 0  },
			{ "2560x1080  (21:9)",       2560, 1080, 64 },
			{ "1024x768   (4:3)",        1024,  768, 0  },
			{ "1080x1920  (mobile)",     1080, 1920, 96 },
		};
	}

	void UIEditorPanel::RegisterMockProfiles()
	{
		m_Mocks = {
			{ "Default",         "Healthy player, level 30, no debuffs." },
			{ "Low HP",          "20% HP, no mana, in combat." },
			{ "Dead Target",     "Target at 0% HP, fading bars." },
			{ "Full Inventory",  "All 32 inventory slots populated." },
			{ "Combat - Boss",   "Targeting a raid boss, multiple buffs." },
		};
	}

	void UIEditorPanel::RegisterThemes()
	{
		m_Themes = {
			{ "Dark",          C(0x101218FF), C(0x1f2530F0), C(0x3a7bd5FF), C(0xeef0f6FF) },
			{ "High Contrast", C(0x000000FF), C(0x0a0a0aFF), C(0xffd24aFF), C(0xffffffFF) },
			{ "Slate",         C(0x182026FF), C(0x222a31F0), C(0x4dd0e1FF), C(0xe6ecefFF) },
		};
	}

	void UIEditorPanel::RegisterLocales()
	{
		m_Locales = {
			{ "en", "English"   },
			{ "es", "Espanol"   },
			{ "de", "Deutsch"   },
			{ "fr", "Francais"  },
			{ "pt", "Portugues" },
		};
	}

	void UIEditorPanel::RebuildCurrentStage()
	{
		if (m_Stages.empty() || !m_Tree)
			return;

		m_StageTime      = 0.0f;
		m_SelectedWidget = nullptr;

		auto root = m_Stages[m_CurrentStage].build();
		m_Tree->SetRoot(std::move(root));
	}

	// =========================================================================
	// Stage builders
	// =========================================================================

	std::unique_ptr<Widget> UIEditorPanel::BuildLoginStage()
	{
		const FontAtlas& font = Onyx::Application::GetInstance().GetUI().GetDefaultFont();

		auto root = std::make_unique<Widget>();
		root->FillSize();

		PushFill(root.get(), MakeRect(0x101218FF, "login-bg"));

		auto card = MakeRect(0x1f2530FF, "login-card");
		card->SetClipsChildren(true);

		PushAnchored(card.get(), MakeText(font, "Onyx Online", 36.0f, 0xeef0f6FF, UIRenderer::TextAlignH::Center),
					 AnchorEdge::Top, glm::vec2(0, 30), glm::vec2(360, 50));

		PushAnchored(card.get(), MakeText(font, "Username", 13.0f, 0xa0a8b8FF),
					 AnchorEdge::TopLeft, glm::vec2(40, 110), glm::vec2(280, 18));
		auto userField = std::make_unique<TextInput>(font, 14.0f);
		userField->SetId("username-field");
		userField->SetPlaceholder("your username");
		userField->SetMaxLength(32);
		PushAnchored(card.get(), std::move(userField),
					 AnchorEdge::TopLeft, glm::vec2(40, 132), glm::vec2(300, 36));

		PushAnchored(card.get(), MakeText(font, "Password", 13.0f, 0xa0a8b8FF),
					 AnchorEdge::TopLeft, glm::vec2(40, 188), glm::vec2(280, 18));
		auto passField = std::make_unique<TextInput>(font, 14.0f);
		passField->SetId("password-field");
		passField->SetPlaceholder("password");
		passField->SetPasswordMode(true);
		passField->SetMaxLength(64);
		PushAnchored(card.get(), std::move(passField),
					 AnchorEdge::TopLeft, glm::vec2(40, 210), glm::vec2(300, 36));

		Widget* rootPtr = root.get();
		auto signIn = std::make_unique<Button>(font, "Sign In", 16.0f);
		signIn->SetId("login-button");
		signIn->ApplyStyle(Button::Style::Primary);
		signIn->SetOnClick([rootPtr] {
			auto* user = dynamic_cast<TextInput*>(rootPtr->FindById("username-field"));
			auto* pass = dynamic_cast<TextInput*>(rootPtr->FindById("password-field"));
			std::cout << "[stage:login] Sign In - user='"
					  << (user ? user->GetText() : std::string("?"))
					  << "' pass.len="
					  << (pass ? pass->GetText().size() : 0)
					  << '\n';
		});
		PushAnchored(card.get(), std::move(signIn),
					 AnchorEdge::TopLeft, glm::vec2(40, 280), glm::vec2(300, 44));

		PushAnchored(card.get(), MakeText(font, "Server: localhost:7000", 12.0f, 0x808898FF, UIRenderer::TextAlignH::Center),
					 AnchorEdge::Bottom, glm::vec2(0, -20), glm::vec2(300, 16));

		card->FixedSize(380, 460);
		card->Anchor(AnchorEdge::Center);
		root->AddChild(std::move(card));

		return root;
	}

	std::unique_ptr<Widget> UIEditorPanel::BuildCharacterSelectStage()
	{
		const FontAtlas& font = Onyx::Application::GetInstance().GetUI().GetDefaultFont();

		auto root = std::make_unique<Widget>();
		root->FillSize();

		PushFill(root.get(), MakeRect(0x141822FF, "charselect-bg"));

		auto header = MakeRect(0x1c2330FF, "charselect-header");
		PushAnchored(header.get(), MakeText(font, "Choose Your Character", 28.0f, 0xeef0f6FF, UIRenderer::TextAlignH::Center),
					 AnchorEdge::Center, glm::vec2(0, 0), glm::vec2(640, 36));
		header->GetLayoutSpec().widthMode = SizeMode::Fill;
		header->GetLayoutSpec().heightMode = SizeMode::Fixed;
		header->GetLayoutSpec().height = 80.0f;
		header->Anchor(AnchorEdge::Top);
		root->AddChild(std::move(header));

		struct CharRow {
			const char* name;
			const char* role;
			uint32_t portraitColor;
		};
		const CharRow chars[] = {
			{ "Thrall",    "Lvl 28 Shaman",  0x6677BBFF },
			{ "Sylvanas",  "Lvl 32 Hunter",  0x5588A0FF },
			{ "Garrosh",   "Lvl 19 Warrior", 0xAA5544FF },
		};

		for (int i = 0; i < 3; ++i)
		{
			std::string id = "char-card-" + std::to_string(i);
			auto card = MakeRect(0x232a3cFF, id.c_str(), true);
			card->SetClipsChildren(true);

			std::string portraitId = "portrait-" + std::to_string(i);
			PushAnchored(card.get(), MakeRect(chars[i].portraitColor, portraitId.c_str()),
						 AnchorEdge::TopLeft, glm::vec2(16, 16), glm::vec2(96, 96));
			PushAnchored(card.get(), MakeText(font, chars[i].name, 22.0f, 0xeef0f6FF),
						 AnchorEdge::TopLeft, glm::vec2(128, 24), glm::vec2(280, 26));
			PushAnchored(card.get(), MakeText(font, chars[i].role, 14.0f, 0xa0a8b8FF),
						 AnchorEdge::TopLeft, glm::vec2(128, 56), glm::vec2(280, 18));

			card->FixedSize(440, 128);
			card->Anchor(AnchorEdge::Top, glm::vec2(0, 140.0f + i * 144.0f));
			root->AddChild(std::move(card));
		}

		auto actions = MakeRect(0x1c2330FF, "charselect-actions");
		PushAnchored(actions.get(), MakeRect(0x3a7bd5FF, "btn-enter-world", true),
					 AnchorEdge::Center, glm::vec2(-120, 0), glm::vec2(200, 44));
		PushAnchored(actions.get(), MakeText(font, "Enter World", 16.0f, 0xffffffFF, UIRenderer::TextAlignH::Center),
					 AnchorEdge::Center, glm::vec2(-120, 6), glm::vec2(200, 28));
		PushAnchored(actions.get(), MakeRect(0x4a5160FF, "btn-create-new", true),
					 AnchorEdge::Center, glm::vec2(120, 0), glm::vec2(200, 44));
		PushAnchored(actions.get(), MakeText(font, "Create New", 16.0f, 0xeef0f6FF, UIRenderer::TextAlignH::Center),
					 AnchorEdge::Center, glm::vec2(120, 6), glm::vec2(200, 28));
		actions->GetLayoutSpec().widthMode = SizeMode::Fill;
		actions->GetLayoutSpec().heightMode = SizeMode::Fixed;
		actions->GetLayoutSpec().height = 80.0f;
		actions->Anchor(AnchorEdge::Bottom);
		root->AddChild(std::move(actions));

		return root;
	}

	std::unique_ptr<Widget> UIEditorPanel::BuildLoadingStage()
	{
		const FontAtlas& font = Onyx::Application::GetInstance().GetUI().GetDefaultFont();

		auto root = std::make_unique<Widget>();
		root->FillSize();

		PushFill(root.get(), MakeRect(0x0a0c10FF, "loading-bg"));

		PushAnchored(root.get(), MakeText(font, "Entering World...", 32.0f, 0xeef0f6FF, UIRenderer::TextAlignH::Center),
					 AnchorEdge::Center, glm::vec2(0, -40), glm::vec2(600, 40));

		// Progress bar track + fill (the fill width is animated in OnUpdate via a custom widget;
		// for now we just show a static 60% fill — the visual is what matters).
		PushAnchored(root.get(), MakeRect(0x252b35FF, "progress-track"),
					 AnchorEdge::Center, glm::vec2(0, 12), glm::vec2(600, 14));
		auto fill = MakeRect(0x4dd0e1FF, "progress-fill");
		// Fill width animated based on m_StageTime — see RebuildCurrentStage / Update.
		// Static here; live anim achievable once we add a custom progress widget (M9).
		PushAnchored(root.get(), std::move(fill),
					 AnchorEdge::Center, glm::vec2(-90, 12), glm::vec2(420, 14));

		PushAnchored(root.get(), MakeText(font, "Loading map: orgrimmar", 14.0f, 0x808898FF, UIRenderer::TextAlignH::Center),
					 AnchorEdge::Center, glm::vec2(0, 48), glm::vec2(600, 18));

		return root;
	}

	std::unique_ptr<Widget> UIEditorPanel::BuildInGameHudStage()
	{
		const FontAtlas& font = Onyx::Application::GetInstance().GetUI().GetDefaultFont();

		auto root = std::make_unique<Widget>();
		root->FillSize();

		// Player frame (top-left)
		auto playerFrame = MakeRect(0x1f2530F0, "player-frame");
		PushAnchored(playerFrame.get(), MakeRect(0x6677BBFF, "player-portrait"),
					 AnchorEdge::TopLeft, glm::vec2(8, 8), glm::vec2(64, 64));
		PushAnchored(playerFrame.get(), MakeText(font, "Thrall", 16.0f, 0xeef0f6FF),
					 AnchorEdge::TopLeft, glm::vec2(80, 12), glm::vec2(160, 18));
		PushAnchored(playerFrame.get(), MakeRect(0x882030FF, "player-hp-bg"),
					 AnchorEdge::TopLeft, glm::vec2(80, 32), glm::vec2(180, 12));
		PushAnchored(playerFrame.get(), MakeRect(0x4ec06aFF, "player-hp-fill"),
					 AnchorEdge::TopLeft, glm::vec2(80, 32), glm::vec2(160, 12));
		PushAnchored(playerFrame.get(), MakeRect(0x102050FF, "player-mp-bg"),
					 AnchorEdge::TopLeft, glm::vec2(80, 48), glm::vec2(180, 8));
		PushAnchored(playerFrame.get(), MakeRect(0x4477e0FF, "player-mp-fill"),
					 AnchorEdge::TopLeft, glm::vec2(80, 48), glm::vec2(140, 8));
		playerFrame->FixedSize(280, 80);
		playerFrame->Anchor(AnchorEdge::TopLeft, glm::vec2(20, 20));
		root->AddChild(std::move(playerFrame));

		// Target frame (top-center)
		auto targetFrame = MakeRect(0x1f2530F0, "target-frame");
		PushAnchored(targetFrame.get(), MakeRect(0x553355FF, "target-portrait"),
					 AnchorEdge::TopLeft, glm::vec2(8, 8), glm::vec2(56, 56));
		PushAnchored(targetFrame.get(), MakeText(font, "Hogger", 14.0f, 0xeef0f6FF),
					 AnchorEdge::TopLeft, glm::vec2(72, 10), glm::vec2(160, 18));
		PushAnchored(targetFrame.get(), MakeRect(0x882030FF, "target-hp-bg"),
					 AnchorEdge::TopLeft, glm::vec2(72, 30), glm::vec2(160, 10));
		PushAnchored(targetFrame.get(), MakeRect(0x4ec06aFF, "target-hp-fill"),
					 AnchorEdge::TopLeft, glm::vec2(72, 30), glm::vec2(96, 10));
		targetFrame->FixedSize(248, 72);
		targetFrame->Anchor(AnchorEdge::Top, glm::vec2(0, 24));
		root->AddChild(std::move(targetFrame));

		// Minimap (top-right)
		auto minimap = MakeRect(0x1f2530F0, "minimap-frame");
		PushAnchored(minimap.get(), MakeRect(0x404858FF, "minimap-canvas"),
					 AnchorEdge::Center, glm::vec2(0, 0), glm::vec2(180, 180));
		minimap->FixedSize(200, 200);
		minimap->Anchor(AnchorEdge::TopRight, glm::vec2(-20, 20));
		root->AddChild(std::move(minimap));

		// Action bar (bottom-center, 12 slots)
		auto actionBar = MakeRect(0x1f2530F0, "action-bar");
		actionBar->SetClipsChildren(true);
		for (int i = 0; i < 12; ++i)
		{
			std::string id = "ability-" + std::to_string(i);
			uint32_t color = (i % 3 == 0) ? 0xa05030FF : ((i % 3 == 1) ? 0x3060a0FF : 0x40a060FF);
			PushAnchored(actionBar.get(), MakeRect(color, id.c_str(), true),
						 AnchorEdge::TopLeft, glm::vec2(8.0f + i * 50.0f, 8), glm::vec2(46, 46));
		}
		actionBar->FixedSize(620, 64);
		actionBar->Anchor(AnchorEdge::Bottom, glm::vec2(0, -20));
		root->AddChild(std::move(actionBar));

		// Chat (bottom-left)
		auto chat = MakeRect(0x1219204A, "chat-frame");
		chat->SetClipsChildren(true);
		PushAnchored(chat.get(), MakeText(font, "[Guild] Thrall: rdy?", 13.0f, 0xb0e0a8FF),
					 AnchorEdge::TopLeft, glm::vec2(8, 8), glm::vec2(360, 16));
		PushAnchored(chat.get(), MakeText(font, "[Party] Sylvanas: pulling", 13.0f, 0xa0c8eaFF),
					 AnchorEdge::TopLeft, glm::vec2(8, 28), glm::vec2(360, 16));
		PushAnchored(chat.get(), MakeText(font, "[Say] Garrosh: lok'tar!", 13.0f, 0xeeeeeeFF),
					 AnchorEdge::TopLeft, glm::vec2(8, 48), glm::vec2(360, 16));
		chat->FixedSize(380, 140);
		chat->Anchor(AnchorEdge::BottomLeft, glm::vec2(20, -20));
		root->AddChild(std::move(chat));

		// Buff row (top-right under minimap)
		auto buffs = std::make_unique<Widget>();
		for (int i = 0; i < 6; ++i)
		{
			uint32_t color = 0x60a070FF;
			std::string id = "buff-" + std::to_string(i);
			PushAnchored(buffs.get(), MakeRect(color, id.c_str()),
						 AnchorEdge::TopLeft, glm::vec2(i * 36.0f, 0), glm::vec2(32, 32));
		}
		buffs->FixedSize(200, 32);
		buffs->Anchor(AnchorEdge::TopRight, glm::vec2(-20, 230));
		root->AddChild(std::move(buffs));

		return root;
	}

	std::unique_ptr<Widget> UIEditorPanel::BuildInGameInventoryStage()
	{
		// Compose: HUD root + modal overlay (we don't yet have a real overlay
		// mechanism in this preview tree; render the modal as a top-z child).
		auto root = BuildInGameHudStage();
		const FontAtlas& font = Onyx::Application::GetInstance().GetUI().GetDefaultFont();

		// Dim layer
		auto dim = MakeRect(0x00000080, "modal-dim");
		dim->FillSize();
		dim->SetZOrder(10);
		root->AddChild(std::move(dim));

		// Inventory modal frame
		auto modal = MakeRect(0x1a2230FF, "inventory-modal");
		modal->SetClipsChildren(true);
		modal->SetZOrder(11);

		PushAnchored(modal.get(), MakeText(font, "Inventory", 22.0f, 0xeef0f6FF),
					 AnchorEdge::TopLeft, glm::vec2(20, 16), glm::vec2(200, 26));

		PushAnchored(modal.get(), MakeRect(0x3a3f4cFF, "close-btn", true),
					 AnchorEdge::TopRight, glm::vec2(-12, 12), glm::vec2(28, 28));
		PushAnchored(modal.get(), MakeText(font, "X", 16.0f, 0xeef0f6FF, UIRenderer::TextAlignH::Center),
					 AnchorEdge::TopRight, glm::vec2(-12, 16), glm::vec2(28, 24));

		// 8 columns × 6 rows of slots
		auto grid = std::make_unique<Widget>();
		grid->SetClipsChildren(true);
		const float slotSize = 44.0f;
		const float slotGap  = 4.0f;
		for (int r = 0; r < 6; ++r)
		{
			for (int c = 0; c < 8; ++c)
			{
				int idx = r * 8 + c;
				uint32_t color = ((idx + r) & 1) ? 0x232a3cFF : 0x2c3548FF;
				if (idx < 14)
					color = 0x4a6080FF; // some "filled" slots tinted
				std::string id = "slot-" + std::to_string(idx);
				PushAnchored(grid.get(), MakeRect(color, id.c_str(), true),
							 AnchorEdge::TopLeft,
							 glm::vec2(c * (slotSize + slotGap), r * (slotSize + slotGap)),
							 glm::vec2(slotSize, slotSize));
			}
		}
		grid->FixedSize(8 * slotSize + 7 * slotGap, 6 * slotSize + 5 * slotGap);
		grid->Anchor(AnchorEdge::TopLeft, glm::vec2(20, 56));
		modal->AddChild(std::move(grid));

		// Footer
		PushAnchored(modal.get(), MakeText(font, "Gold: 1,247", 14.0f, 0xffd24aFF),
					 AnchorEdge::BottomLeft, glm::vec2(20, -22), glm::vec2(200, 18));
		PushAnchored(modal.get(), MakeText(font, "14/48 slots", 14.0f, 0xa0a8b8FF, UIRenderer::TextAlignH::Right),
					 AnchorEdge::BottomRight, glm::vec2(-20, -22), glm::vec2(200, 18));

		modal->FixedSize(420, 380);
		modal->Anchor(AnchorEdge::Center);
		root->AddChild(std::move(modal));

		// Tooltip (clip-escape would handle this in production; we model it
		// as another high-z child for now)
		auto tooltip = MakeRect(0x222a35F0, "buff-tooltip");
		tooltip->SetZOrder(20);
		PushAnchored(tooltip.get(), MakeText(font, "Blessing of Kings", 14.0f, 0xffd24aFF),
					 AnchorEdge::TopLeft, glm::vec2(10, 8), glm::vec2(220, 18));
		PushAnchored(tooltip.get(), MakeText(font, "+10% to all stats for 30 min.", 12.0f, 0xc0c8d0FF),
					 AnchorEdge::TopLeft, glm::vec2(10, 30), glm::vec2(220, 16));
		tooltip->FixedSize(240, 60);
		tooltip->Anchor(AnchorEdge::TopRight, glm::vec2(-200, 280));
		root->AddChild(std::move(tooltip));

		return root;
	}

	std::unique_ptr<Widget> UIEditorPanel::BuildCombatLowHpStage()
	{
		auto root = BuildInGameHudStage();
		const FontAtlas& font = Onyx::Application::GetInstance().GetUI().GetDefaultFont();

		// Replace player HP fill with a low-HP red bar (find by id).
		if (auto* fill = root->FindById("player-hp-fill"))
		{
			if (auto* rect = dynamic_cast<DebugRect*>(fill))
			{
				rect->SetBaseColor(C(0xc02828FF));
			}
			fill->GetLayoutSpec().width = 36.0f; // ~22%
		}

		// Cast bar (centered above action bar)
		auto castBar = MakeRect(0x1f2530F0, "cast-bar");
		PushAnchored(castBar.get(), MakeRect(0x3060a0FF, "cast-fill"),
					 AnchorEdge::TopLeft, glm::vec2(0, 0), glm::vec2(180, 16));
		PushAnchored(castBar.get(), MakeText(font, "Lightning Bolt  1.8s", 12.0f, 0xeef0f6FF),
					 AnchorEdge::TopLeft, glm::vec2(8, 0), glm::vec2(280, 16));
		castBar->FixedSize(300, 16);
		castBar->Anchor(AnchorEdge::Bottom, glm::vec2(0, -110));
		root->AddChild(std::move(castBar));

		// Floating combat text
		PushAnchored(root.get(), MakeText(font, "1,247", 28.0f, 0xffd24aFF, UIRenderer::TextAlignH::Center),
					 AnchorEdge::Center, glm::vec2(40, -160), glm::vec2(200, 36));

		return root;
	}

	std::unique_ptr<Widget> UIEditorPanel::BuildDeathScreenStage()
	{
		const FontAtlas& font = Onyx::Application::GetInstance().GetUI().GetDefaultFont();

		auto root = std::make_unique<Widget>();
		root->FillSize();

		PushFill(root.get(), MakeRect(0x150a0aF0, "death-overlay"));

		PushAnchored(root.get(), MakeText(font, "You have died", 56.0f, 0xc02828FF, UIRenderer::TextAlignH::Center),
					 AnchorEdge::Center, glm::vec2(0, -60), glm::vec2(900, 64));

		PushAnchored(root.get(), MakeRect(0x3a7bd5FF, "btn-respawn", true),
					 AnchorEdge::Center, glm::vec2(-110, 24), glm::vec2(200, 48));
		PushAnchored(root.get(), MakeText(font, "Respawn", 18.0f, 0xffffffFF, UIRenderer::TextAlignH::Center),
					 AnchorEdge::Center, glm::vec2(-110, 32), glm::vec2(200, 32));

		PushAnchored(root.get(), MakeRect(0x4a5160FF, "btn-graveyard", true),
					 AnchorEdge::Center, glm::vec2(110, 24), glm::vec2(200, 48));
		PushAnchored(root.get(), MakeText(font, "Release Spirit", 18.0f, 0xeef0f6FF, UIRenderer::TextAlignH::Center),
					 AnchorEdge::Center, glm::vec2(110, 32), glm::vec2(200, 32));

		return root;
	}

	// =========================================================================
	// Rendering
	// =========================================================================

	void UIEditorPanel::EnsureFramebuffer(int w, int h)
	{
		if (w <= 0 || h <= 0) return;
		if (!m_Framebuffer) return;
		if (m_FbWidth == w && m_FbHeight == h) return;

		m_Framebuffer->Resize(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
		m_FbWidth  = w;
		m_FbHeight = h;
	}

	void UIEditorPanel::RenderCanvasToFramebuffer(int canvasWidth, int canvasHeight, float deltaTime)
	{
		if (canvasWidth <= 0 || canvasHeight <= 0) return;
		EnsureFramebuffer(canvasWidth, canvasHeight);

		if (!m_Tree || !m_Tree->GetRoot()) return;

		if (!m_RendererInitialized)
		{
			m_Renderer->Init();
			m_RendererInitialized = true;
		}

		// Run layout + per-widget OnUpdate
		m_Tree->SetViewport(glm::vec2(canvasWidth, canvasHeight));
		m_Tree->Update(deltaTime);

		// Save GL state we touch so the rest of the editor's rendering isn't
		// affected by leftover blend / scissor / framebuffer bindings.
		GLint   prevFB = 0;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFB);
		GLint   prevVP[4];
		glGetIntegerv(GL_VIEWPORT, prevVP);
		GLboolean prevBlend   = glIsEnabled(GL_BLEND);
		GLboolean prevDepth   = glIsEnabled(GL_DEPTH_TEST);
		GLboolean prevScissor = glIsEnabled(GL_SCISSOR_TEST);

		m_Framebuffer->Bind(); // sets viewport + binds FBO

		const auto& theme = m_Themes[m_CurrentTheme];
		glClearColor(theme.background.r, theme.background.g, theme.background.b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Premultiplied-alpha blend mode (matches UIRenderer's vertex output).
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_SCISSOR_TEST);

		m_Renderer->BeginFrame(glm::ivec2(canvasWidth, canvasHeight));
		m_Tree->Draw(*m_Renderer);

		if (m_ShowSafeArea && m_CurrentRes < static_cast<int>(m_Resolutions.size()))
		{
			DrawSafeAreaOverlay(canvasWidth, canvasHeight, m_Resolutions[m_CurrentRes].safeAreaInset);
		}
		if (m_SelectedWidget && m_SelectedWidget->IsVisible())
		{
			DrawSelectionHandles();
		}

		m_Renderer->EndFrame();

		m_Framebuffer->Resolve();
		m_Framebuffer->UnBind();

		// Restore prior GL state.
		glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFB));
		glViewport(prevVP[0], prevVP[1], prevVP[2], prevVP[3]);
		if (prevBlend)   glEnable(GL_BLEND);   else glDisable(GL_BLEND);
		if (prevDepth)   glEnable(GL_DEPTH_TEST);   else glDisable(GL_DEPTH_TEST);
		if (prevScissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
	}

	void UIEditorPanel::DrawSafeAreaOverlay(int canvasWidth, int canvasHeight, int safeAreaInset)
	{
		if (!m_Renderer) return;

		const Color frameColor{1.0f, 0.65f, 0.0f, 0.7f};
		const Color edgeColor{0.40f, 0.45f, 0.55f, 0.5f};
		const float thickness = 1.5f;

		const float w = static_cast<float>(canvasWidth);
		const float h = static_cast<float>(canvasHeight);

		// Outer canvas edge.
		m_Renderer->DrawRect(Rect2D{glm::vec2(0, 0),         glm::vec2(w, thickness)},     edgeColor);
		m_Renderer->DrawRect(Rect2D{glm::vec2(0, h - thickness), glm::vec2(w, h)},          edgeColor);
		m_Renderer->DrawRect(Rect2D{glm::vec2(0, 0),         glm::vec2(thickness, h)},     edgeColor);
		m_Renderer->DrawRect(Rect2D{glm::vec2(w - thickness, 0), glm::vec2(w, h)},          edgeColor);

		// Inset frame at safe-area distance.
		if (safeAreaInset > 0)
		{
			const float i = static_cast<float>(safeAreaInset);
			m_Renderer->DrawRect(Rect2D{glm::vec2(i, i),                 glm::vec2(w - i, i + thickness)},     frameColor);
			m_Renderer->DrawRect(Rect2D{glm::vec2(i, h - i - thickness), glm::vec2(w - i, h - i)},             frameColor);
			m_Renderer->DrawRect(Rect2D{glm::vec2(i, i),                 glm::vec2(i + thickness, h - i)},     frameColor);
			m_Renderer->DrawRect(Rect2D{glm::vec2(w - i - thickness, i), glm::vec2(w - i, h - i)},             frameColor);
		}
	}

	void UIEditorPanel::DrawSelectionHandles()
	{
		if (!m_Renderer || !m_SelectedWidget) return;

		const Rect2D b = m_SelectedWidget->GetBounds();
		if (b.Empty()) return;

		const Color handleColor{0.40f, 0.85f, 1.0f, 0.85f};
		const float t = 1.5f;

		// Frame
		m_Renderer->DrawRect(Rect2D{glm::vec2(b.min.x, b.min.y), glm::vec2(b.max.x, b.min.y + t)},   handleColor);
		m_Renderer->DrawRect(Rect2D{glm::vec2(b.min.x, b.max.y - t), glm::vec2(b.max.x, b.max.y)},   handleColor);
		m_Renderer->DrawRect(Rect2D{glm::vec2(b.min.x, b.min.y), glm::vec2(b.min.x + t, b.max.y)},   handleColor);
		m_Renderer->DrawRect(Rect2D{glm::vec2(b.max.x - t, b.min.y), glm::vec2(b.max.x, b.max.y)},   handleColor);

		// Corner squares
		const float hs = 8.0f;
		auto corner = [&](float cx, float cy) {
			m_Renderer->DrawRect(Rect2D{glm::vec2(cx - hs * 0.5f, cy - hs * 0.5f),
										glm::vec2(cx + hs * 0.5f, cy + hs * 0.5f)}, handleColor);
		};
		corner(b.min.x, b.min.y);
		corner(b.max.x, b.min.y);
		corner(b.min.x, b.max.y);
		corner(b.max.x, b.max.y);
	}

	// =========================================================================
	// ImGui sub-panels
	// =========================================================================

	void UIEditorPanel::DrawTopToolbar()
	{
		// Row 1: stage / mock / theme / locale
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Stage:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(220);
		if (!m_Stages.empty() &&
			ImGui::BeginCombo("##stage", m_Stages[m_CurrentStage].name.c_str()))
		{
			for (size_t i = 0; i < m_Stages.size(); ++i)
			{
				bool selected = static_cast<int>(i) == m_CurrentStage;
				if (ImGui::Selectable(m_Stages[i].name.c_str(), selected))
				{
					m_CurrentStage = static_cast<int>(i);
					RebuildCurrentStage();
				}
				if (ImGui::IsItemHovered() && !m_Stages[i].description.empty())
					ImGui::SetTooltip("%s", m_Stages[i].description.c_str());
			}
			ImGui::EndCombo();
		}

		ImGui::SameLine(0, 16);
		ImGui::TextUnformatted("Mock:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(180);
		if (!m_Mocks.empty() &&
			ImGui::BeginCombo("##mock", m_Mocks[m_CurrentMock].name.c_str()))
		{
			for (size_t i = 0; i < m_Mocks.size(); ++i)
			{
				bool selected = static_cast<int>(i) == m_CurrentMock;
				if (ImGui::Selectable(m_Mocks[i].name.c_str(), selected))
					m_CurrentMock = static_cast<int>(i);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s\n(preview-only; M7 wires real bindings)", m_Mocks[i].description.c_str());
			}
			ImGui::EndCombo();
		}

		ImGui::SameLine(0, 16);
		ImGui::TextUnformatted("Theme:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(140);
		if (!m_Themes.empty() &&
			ImGui::BeginCombo("##theme", m_Themes[m_CurrentTheme].name.c_str()))
		{
			for (size_t i = 0; i < m_Themes.size(); ++i)
			{
				bool selected = static_cast<int>(i) == m_CurrentTheme;
				ImGui::ColorButton(("##swatch" + std::to_string(i)).c_str(),
								   ToImVec4(m_Themes[i].accent),
								   ImGuiColorEditFlags_NoTooltip,
								   ImVec2(14, 14));
				ImGui::SameLine();
				if (ImGui::Selectable(m_Themes[i].name.c_str(), selected))
					m_CurrentTheme = static_cast<int>(i);
			}
			ImGui::EndCombo();
		}

		ImGui::SameLine(0, 16);
		ImGui::TextUnformatted("Locale:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(140);
		if (!m_Locales.empty() &&
			ImGui::BeginCombo("##locale", m_Locales[m_CurrentLocale].code.c_str()))
		{
			for (size_t i = 0; i < m_Locales.size(); ++i)
			{
				bool selected = static_cast<int>(i) == m_CurrentLocale;
				std::string label = m_Locales[i].code + " - " + m_Locales[i].name;
				if (ImGui::Selectable(label.c_str(), selected))
					m_CurrentLocale = static_cast<int>(i);
			}
			ImGui::EndCombo();
		}

		// Row 2: resolution / toggles / live indicator
		ImGui::TextUnformatted("Res:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(220);
		if (!m_Resolutions.empty() &&
			ImGui::BeginCombo("##res", m_Resolutions[m_CurrentRes].name.c_str()))
		{
			for (size_t i = 0; i < m_Resolutions.size(); ++i)
			{
				bool selected = static_cast<int>(i) == m_CurrentRes;
				if (ImGui::Selectable(m_Resolutions[i].name.c_str(), selected))
					m_CurrentRes = static_cast<int>(i);
			}
			ImGui::EndCombo();
		}

		ImGui::SameLine(0, 16);
		ImGui::Checkbox("Safe area", &m_ShowSafeArea);
		ImGui::SameLine();
		ImGui::Checkbox("Z-X-Ray", &m_ShowZXRay);
		ImGui::SameLine();
		ImGui::Checkbox("Overflow", &m_ShowOverflow);
		ImGui::SameLine();
		ImGui::Checkbox("Grid", &m_ShowGrid);

		ImGui::SameLine(0, 24);
		ImVec4 dotColor = m_LiveClientConnected
							  ? ImVec4(0.30f, 0.90f, 0.40f, 1.0f)
							  : ImVec4(0.55f, 0.58f, 0.65f, 1.0f);
		ImDrawList* dl = ImGui::GetWindowDrawList();
		ImVec2      cursor = ImGui::GetCursorScreenPos();
		cursor.y += ImGui::GetTextLineHeight() * 0.5f;
		cursor.x += 6.0f;
		dl->AddCircleFilled(cursor, 5.5f, ImGui::ColorConvertFloat4ToU32(dotColor));
		ImGui::Dummy(ImVec2(16, 0));
		ImGui::SameLine();
		ImGui::TextDisabled("%s",
							m_LiveClientConnected ? "Live: MMOClient connected"
												   : "Live: not connected (M5 wires hot-reload)");
	}

	void UIEditorPanel::DrawHierarchyColumn()
	{
		ImGui::TextUnformatted("Hierarchy");
		ImGui::Separator();

		if (m_Tree && m_Tree->GetRoot())
		{
			ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 14.0f);
			DrawHierarchyNode(m_Tree->GetRoot());
			ImGui::PopStyleVar();
		}
		else
		{
			ImGui::TextDisabled("No stage built yet — font may still be loading.");
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextUnformatted("Components");
		const char* components[] = {
			"unit_frame.ui", "tooltip.ui", "item_icon.ui", "drag_ghost.ui"
		};
		for (const char* c : components)
			ImGui::BulletText("%s", c);
		ImGui::TextDisabled("(component palette wires up with M5)");
	}

	void UIEditorPanel::DrawHierarchyNode(Widget* widget)
	{
		if (!widget) return;

		const auto& children = widget->GetChildren();
		bool        leaf = children.empty();

		const char* typeTag = "<widget>";
		if (dynamic_cast<DebugRect*>(widget))
			typeTag = "<rect>";
		else if (dynamic_cast<Label*>(widget))
			typeTag = "<label>";

		std::string label = std::string(typeTag);
		if (!widget->GetId().empty())
			label += "  " + widget->GetId();

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
								   ImGuiTreeNodeFlags_OpenOnDoubleClick |
								   ImGuiTreeNodeFlags_SpanAvailWidth;
		if (leaf)
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		if (widget == m_SelectedWidget)
			flags |= ImGuiTreeNodeFlags_Selected;

		bool nodeOpen = ImGui::TreeNodeEx(static_cast<void*>(widget), flags, "%s", label.c_str());
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
			m_SelectedWidget = widget;

		if (nodeOpen && !leaf)
		{
			for (const auto& child : children)
				DrawHierarchyNode(child.get());
			ImGui::TreePop();
		}
	}

	void UIEditorPanel::DrawCanvasColumn(float availableWidth, float availableHeight)
	{
		if (m_Resolutions.empty()) return;
		const auto& res = m_Resolutions[m_CurrentRes];
		const float resAspect = static_cast<float>(res.width) / static_cast<float>(res.height);

		const float chromeH = 28.0f;
		const float canvasH = std::max(80.0f, availableHeight - chromeH - 8.0f);

		float displayW = availableWidth;
		float displayH = displayW / resAspect;
		if (displayH > canvasH)
		{
			displayH = canvasH;
			displayW = displayH * resAspect;
		}

		float deltaTime = ImGui::GetIO().DeltaTime;
		m_StageTime += deltaTime;
		RenderCanvasToFramebuffer(res.width, res.height, deltaTime);

		// Center within column.
		ImVec2 cursor = ImGui::GetCursorPos();
		float  offsetX = std::max(0.0f, (availableWidth - displayW) * 0.5f);
		float  offsetY = std::max(0.0f, (canvasH - displayH) * 0.5f);
		ImGui::SetCursorPos(ImVec2(cursor.x + offsetX, cursor.y + offsetY));

		ImVec2 imageOrigin = ImGui::GetCursorScreenPos();
		ImVec2 imageSize(displayW, displayH);

		if (m_Framebuffer && m_Framebuffer->GetColorBufferID() != 0)
		{
			ImGui::Image(
				static_cast<ImTextureID>(static_cast<uintptr_t>(m_Framebuffer->GetColorBufferID())),
				imageSize,
				ImVec2(0, 1), ImVec2(1, 0));
		}
		else
		{
			ImGui::Dummy(imageSize);
		}

		DispatchInputFromImGui(imageOrigin, imageSize);

		// Canvas chrome.
		ImGui::SetCursorPos(ImVec2(cursor.x, cursor.y + canvasH));
		ImGui::Separator();
		ImGui::Text("%d x %d  -  %s     |     Stage: %s     |     Mock: %s",
					res.width, res.height, res.name.c_str(),
					m_Stages.empty() ? "-" : m_Stages[m_CurrentStage].name.c_str(),
					m_Mocks.empty()  ? "-" : m_Mocks[m_CurrentMock].name.c_str());
	}

	void UIEditorPanel::DrawInspectorColumn()
	{
		ImGui::TextUnformatted("Inspector");
		ImGui::Separator();

		if (!m_SelectedWidget)
		{
			ImGui::TextDisabled("Select a widget in the hierarchy or click on the canvas.");
			return;
		}

		DrawWidgetInspector(m_SelectedWidget);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextUnformatted("Style cascade");
		DrawStyleCascadePlaceholder(m_SelectedWidget);
	}

	void UIEditorPanel::DrawWidgetInspector(Widget* widget)
	{
		// Identity
		{
			char buf[128] = {};
			std::snprintf(buf, sizeof(buf), "%s", widget->GetId().c_str());
			if (ImGui::InputText("id", buf, sizeof(buf)))
				widget->SetId(buf);
		}

		const Rect2D& b = widget->GetBounds();
		ImGui::Text("bounds  %.0f, %.0f -> %.0f, %.0f", b.min.x, b.min.y, b.max.x, b.max.y);
		ImGui::Text("size    %.0f x %.0f", b.Width(), b.Height());

		ImGui::Separator();
		ImGui::TextUnformatted("Layout");

		LayoutSpec& spec = widget->GetLayoutSpec();

		const char* sizeModes[] = {"Fixed", "Fill", "Percent", "Aspect"};
		{
			int wm = static_cast<int>(spec.widthMode);
			ImGui::SetNextItemWidth(80);
			if (ImGui::Combo("w-mode", &wm, sizeModes, IM_ARRAYSIZE(sizeModes)))
				spec.widthMode = static_cast<SizeMode>(wm);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(80);
			ImGui::DragFloat("##width", &spec.width, 1.0f, 0.0f, 4096.0f, "%.0f");
		}
		{
			int hm = static_cast<int>(spec.heightMode);
			ImGui::SetNextItemWidth(80);
			if (ImGui::Combo("h-mode", &hm, sizeModes, IM_ARRAYSIZE(sizeModes)))
				spec.heightMode = static_cast<SizeMode>(hm);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(80);
			ImGui::DragFloat("##height", &spec.height, 1.0f, 0.0f, 4096.0f, "%.0f");
		}
		{
			const char* anchors[] = {"TopLeft", "Top", "TopRight", "Left", "Center",
									 "Right", "BottomLeft", "Bottom", "BottomRight"};
			int a = static_cast<int>(spec.anchor);
			ImGui::SetNextItemWidth(140);
			if (ImGui::Combo("anchor", &a, anchors, IM_ARRAYSIZE(anchors)))
				spec.anchor = static_cast<AnchorEdge>(a);
		}
		{
			ImGui::SetNextItemWidth(160);
			ImGui::DragFloat2("offset", &spec.anchorOffset.x, 1.0f, -4096.0f, 4096.0f, "%.0f");
		}
		{
			ImGui::SetNextItemWidth(200);
			ImGui::DragFloat4("pad t/r/b/l", &spec.padding.x, 0.5f, 0.0f, 256.0f, "%.0f");
		}

		ImGui::Separator();
		ImGui::TextUnformatted("Flags");
		bool visible = widget->IsVisible();
		if (ImGui::Checkbox("Visible", &visible)) widget->SetVisible(visible);
		bool clip = widget->ClipsChildren();
		if (ImGui::Checkbox("Clips children", &clip)) widget->SetClipsChildren(clip);
		bool interactive = widget->IsInteractive();
		if (ImGui::Checkbox("Interactive", &interactive)) widget->SetInteractive(interactive);
		bool focusable = widget->IsFocusable();
		if (ImGui::Checkbox("Focusable", &focusable)) widget->SetFocusable(focusable);
		int z = widget->GetZOrder();
		ImGui::SetNextItemWidth(80);
		if (ImGui::DragInt("z-order", &z))
			widget->SetZOrder(z);

		// Type-specific
		if (auto* dbg = dynamic_cast<DebugRect*>(widget))
		{
			ImGui::Separator();
			ImGui::TextUnformatted("DebugRect");
			Color  c = dbg->GetBaseColor();
			ImVec4 v(c.r, c.g, c.b, c.a);
			if (ImGui::ColorEdit4("base color", &v.x,
								  ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_DisplayHex))
				dbg->SetBaseColor(Color(v.x, v.y, v.z, v.w));

			char lbl[128] = {};
			std::snprintf(lbl, sizeof(lbl), "%s", dbg->GetLabel().c_str());
			if (ImGui::InputText("label", lbl, sizeof(lbl)))
				dbg->SetLabel(lbl);

			ImGui::Text("clicks: %u", dbg->GetClickCount());
		}
		else if (auto* lbl = dynamic_cast<Label*>(widget))
		{
			ImGui::Separator();
			ImGui::TextUnformatted("Label");

			char buf[256] = {};
			std::snprintf(buf, sizeof(buf), "%s", lbl->GetText().c_str());
			if (ImGui::InputText("text", buf, sizeof(buf)))
				lbl->SetText(buf);

			float pxSize = lbl->GetPixelSize();
			if (ImGui::DragFloat("px size", &pxSize, 0.5f, 6.0f, 96.0f, "%.1f"))
				lbl->SetPixelSize(pxSize);

			Color  c = lbl->GetColor();
			ImVec4 v(c.r, c.g, c.b, c.a);
			if (ImGui::ColorEdit4("color", &v.x,
								  ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_DisplayHex))
				lbl->SetColor(Color(v.x, v.y, v.z, v.w));

			const char* alignNames[] = {"Left", "Center", "Right"};
			int         a = static_cast<int>(lbl->GetAlign());
			if (ImGui::Combo("align", &a, alignNames, IM_ARRAYSIZE(alignNames)))
				lbl->SetAlign(static_cast<UIRenderer::TextAlignH>(a));
		}
	}

	void UIEditorPanel::DrawStyleCascadePlaceholder(Widget* widget)
	{
		(void)widget;
		ImGui::TextDisabled("(theme cascade — wires with M6 theme JSON)");
		ImGui::TextDisabled("layers (resolution order):");
		ImGui::BulletText("explicit attribute");
		ImGui::BulletText("class");
		ImGui::BulletText("pseudo-state");
	}

	void UIEditorPanel::DrawPaletteStrip()
	{
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Palette");
		ImGui::SameLine(0, 16);
		ImGui::TextDisabled("(drag-to-canvas wires with M5 .ui round-trip)");

		auto category = [](const char* label, std::initializer_list<const char*> items) {
			ImGui::TextDisabled("%-12s", label);
			ImGui::SameLine();
			for (const char* item : items)
			{
				ImGui::SmallButton(item);
				ImGui::SameLine();
			}
			ImGui::NewLine();
		};

		category("Layout",    { "Stack", "Grid", "Container", "ScrollView", "Spacer", "TabView" });
		category("Display",   { "Label", "Image", "Icon", "ProgressBar", "RoundedPanel", "Divider" });
		category("Input",     { "Button", "Slider", "Checkbox", "Dropdown", "TextInput", "RadioGroup" });
		category("Composite", { "Tooltip", "Modal", "Popup", "Toast", "ContextMenu" });
		category("World",     { "Nameplate", "OverheadBar", "FloatingText", "WorldMarker" });
	}

	void UIEditorPanel::DrawBottomTabs()
	{
		if (ImGui::BeginTabBar("UIE_BottomTabs", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem("Files"))
			{
				ImGui::TextDisabled("File browser will list MMOGame/assets/ui/.");
				ImGui::TextDisabled("Wires up with M5 (.ui loader + efsw watcher).");
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Themes"))
			{
				for (size_t i = 0; i < m_Themes.size(); ++i)
				{
					ImGui::ColorButton(("##swt" + std::to_string(i)).c_str(),
									   ToImVec4(m_Themes[i].accent),
									   ImGuiColorEditFlags_NoTooltip,
									   ImVec2(20, 20));
					ImGui::SameLine();
					bool selected = static_cast<int>(i) == m_CurrentTheme;
					if (ImGui::Selectable(m_Themes[i].name.c_str(), selected))
						m_CurrentTheme = static_cast<int>(i);
				}
				ImGui::TextDisabled("(theme JSON parser wires with M6)");
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Locales"))
			{
				for (size_t i = 0; i < m_Locales.size(); ++i)
				{
					std::string label = m_Locales[i].code + " - " + m_Locales[i].name;
					bool        selected = static_cast<int>(i) == m_CurrentLocale;
					if (ImGui::Selectable(label.c_str(), selected))
						m_CurrentLocale = static_cast<int>(i);
				}
				ImGui::TextDisabled("(tr: lookup + plural rules wire with M10)");
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Bindings Mocks"))
			{
				for (size_t i = 0; i < m_Mocks.size(); ++i)
				{
					bool selected = static_cast<int>(i) == m_CurrentMock;
					if (ImGui::Selectable(m_Mocks[i].name.c_str(), selected))
						m_CurrentMock = static_cast<int>(i);
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("%s", m_Mocks[i].description.c_str());
				}
				ImGui::TextDisabled("(binding context + pending/stale wrappers wire with M7)");
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Sockets"))
			{
				ImGui::TextDisabled("World-attached socket overlay.");
				ImGui::TextDisabled("Editor3D ViewportPanel already has the engine debug overlay (m_ShowSocketDebug).");
				ImGui::TextDisabled("This tab will mirror it for world-attached widgets selected here (M11).");
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}

	// =========================================================================
	// Input forwarding
	// =========================================================================

	void UIEditorPanel::DispatchInputFromImGui(const ImVec2& imageOrigin, const ImVec2& imageSize)
	{
		if (!m_Tree || !m_Tree->GetRoot()) return;
		if (imageSize.x <= 0.0f || imageSize.y <= 0.0f) return;

		ImGuiIO& io = ImGui::GetIO();

		const glm::vec2 mouseScreen{io.MousePos.x, io.MousePos.y};
		const glm::vec2 originV{imageOrigin.x, imageOrigin.y};
		const glm::vec2 sizeV{imageSize.x, imageSize.y};
		const glm::vec2 imageLocal = mouseScreen - originV;
		const glm::vec2 canvasSize{static_cast<float>(m_FbWidth), static_cast<float>(m_FbHeight)};
		const glm::vec2 mouseCanvas = imageLocal * (canvasSize / sizeV);

		const bool isHovered = ImGui::IsItemHovered();

		ModifierFlags mods = 0;
		if (io.KeyShift) mods |= static_cast<uint16_t>(ModifierFlag::Shift);
		if (io.KeyCtrl)  mods |= static_cast<uint16_t>(ModifierFlag::Ctrl);
		if (io.KeyAlt)   mods |= static_cast<uint16_t>(ModifierFlag::Alt);
		if (io.KeySuper) mods |= static_cast<uint16_t>(ModifierFlag::Super);

		const bool currButtons[3] = {
			ImGui::IsMouseDown(ImGuiMouseButton_Left),
			ImGui::IsMouseDown(ImGuiMouseButton_Right),
			ImGui::IsMouseDown(ImGuiMouseButton_Middle),
		};

		// MouseMove
		if (m_PrevMouseValid && (mouseCanvas != m_PrevMouse))
		{
			if (isHovered || m_Tree->GetCapture())
			{
				InputEvent e{};
				e.type     = InputEventType::MouseMove;
				e.position = mouseCanvas;
				e.delta    = mouseCanvas - m_PrevMouse;
				e.mods     = mods;
				m_Tree->DispatchInput(e);
			}
		}

		// Edge events
		for (int b = 0; b < 3; ++b)
		{
			if (currButtons[b] && !m_PrevButtons[b])
			{
				if (isHovered)
				{
					InputEvent e{};
					e.type     = InputEventType::MouseDown;
					e.position = mouseCanvas;
					e.button   = static_cast<MouseButton>(b);
					e.mods     = mods;
					m_Tree->DispatchInput(e);

					if (b == 0 && m_Tree->GetRoot())
					{
						Widget* hit = m_Tree->GetRoot()->HitTest(mouseCanvas);
						if (hit) m_SelectedWidget = hit;
					}
				}
			}
			else if (!currButtons[b] && m_PrevButtons[b])
			{
				InputEvent e{};
				e.type     = InputEventType::MouseUp;
				e.position = mouseCanvas;
				e.button   = static_cast<MouseButton>(b);
				e.mods     = mods;
				m_Tree->DispatchInput(e);
			}
		}

		// Wheel
		if (isHovered && io.MouseWheel != 0.0f)
		{
			InputEvent e{};
			e.type     = InputEventType::MouseWheel;
			e.position = mouseCanvas;
			e.delta    = glm::vec2(0.0f, io.MouseWheel * 30.0f);
			e.mods     = mods;
			m_Tree->DispatchInput(e);
		}

		// Keyboard forwarding for focused TextInput. Gate on the panel's
		// window hierarchy having focus so we don't steal keys from other
		// panels, and on our tree owning a focused widget.
		const bool panelFocused =
			ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
		if (panelFocused && m_Tree->GetFocus())
		{
			// Char input — ImGui buffers typed unicode in InputQueueCharacters.
			for (int i = 0; i < io.InputQueueCharacters.Size; ++i)
			{
				const ImWchar ch = io.InputQueueCharacters[i];
				if (ch == 0) continue;
				InputEvent ce{};
				ce.type      = InputEventType::Char;
				ce.character = static_cast<uint32_t>(ch);
				ce.mods      = mods;
				m_Tree->DispatchInput(ce);
			}

			// Editing keys — map ImGui key to GLFW key code (what
			// InputEvent.keyCode carries). `repeat=true` so held arrows
			// auto-repeat.
			static const struct { ImGuiKey key; uint32_t glfw; } kKeyMap[] = {
				{ ImGuiKey_Tab,         258 },
				{ ImGuiKey_LeftArrow,   263 },
				{ ImGuiKey_RightArrow,  262 },
				{ ImGuiKey_Home,        268 },
				{ ImGuiKey_End,         269 },
				{ ImGuiKey_Backspace,   259 },
				{ ImGuiKey_Delete,      261 },
				{ ImGuiKey_Enter,       257 },
				{ ImGuiKey_KeypadEnter, 335 },
				{ ImGuiKey_Space,       32  },
				{ ImGuiKey_Escape,      256 },
			};
			for (const auto& m : kKeyMap)
			{
				if (ImGui::IsKeyPressed(m.key, true))
				{
					InputEvent ke{};
					ke.type    = InputEventType::KeyDown;
					ke.keyCode = m.glfw;
					ke.mods    = mods;
					m_Tree->DispatchInput(ke);
				}
			}
		}

		m_PrevMouse      = mouseCanvas;
		m_PrevButtons[0] = currButtons[0];
		m_PrevButtons[1] = currButtons[1];
		m_PrevButtons[2] = currButtons[2];
		m_PrevMouseValid = true;
	}

	// =========================================================================
	// OnImGuiRender — orchestrator
	// =========================================================================

	void UIEditorPanel::OnImGuiRender()
	{
		// Lazy-build the current stage once the shared FontAtlas is loaded.
		// The main UI Manager loads it on its first Render(); typically frame 1-2.
		if (m_Tree && !m_Tree->GetRoot() &&
			Onyx::Application::GetInstance().GetUI().DefaultFontLoaded())
		{
			RebuildCurrentStage();
		}

		if (!ImGui::Begin(GetName().c_str(), &m_IsOpen, ImGuiWindowFlags_NoCollapse))
		{
			ImGui::End();
			return;
		}

		DrawTopToolbar();
		ImGui::Separator();

		const ImVec2 avail        = ImGui::GetContentRegionAvail();
		const float  bottomHeight = 220.0f;
		const float  bodyHeight   = std::max(180.0f, avail.y - bottomHeight - 8.0f);

		if (ImGui::BeginTable("UIE_Body", 3,
							  ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV,
							  ImVec2(0, bodyHeight)))
		{
			ImGui::TableSetupColumn("Hierarchy", ImGuiTableColumnFlags_WidthFixed, 240.0f);
			ImGui::TableSetupColumn("Canvas",    ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthFixed, 320.0f);

			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::BeginChild("UIE_HierarchyChild", ImVec2(0, 0), false);
			DrawHierarchyColumn();
			ImGui::EndChild();

			ImGui::TableSetColumnIndex(1);
			const ImVec2 colSize = ImGui::GetContentRegionAvail();
			ImGui::BeginChild("UIE_CanvasChild", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);
			DrawCanvasColumn(colSize.x, colSize.y);
			ImGui::EndChild();

			ImGui::TableSetColumnIndex(2);
			ImGui::BeginChild("UIE_InspectorChild", ImVec2(0, 0), false);
			DrawInspectorColumn();
			ImGui::EndChild();

			ImGui::EndTable();
		}

		ImGui::Separator();

		ImGui::BeginChild("UIE_BottomStrip", ImVec2(0, 0), true);
		DrawPaletteStrip();
		ImGui::Separator();
		DrawBottomTabs();
		ImGui::EndChild();

		ImGui::End();
	}

} // namespace MMO
