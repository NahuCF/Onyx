#pragma once

#include "Source/UI/Color.h"
#include "Source/UI/DragDrop.h"
#include "Source/UI/Widget.h"

#include <functional>
#include <glm/glm.hpp>

namespace Onyx { class Texture; }

namespace Onyx::UI {

	// A slot that is both a drag source and a drop target — exactly an
	// action-bar / inventory cell. Occupied slots drag their payload; any slot
	// can receive a compatible drop. Phase-2 action bar. Non-owning texture.
	class DragSlot : public Widget
	{
	public:
		DragSlot();

		// Registers/unregisters with the context (required for drag+drop).
		void SetContext(DragContext* c);

		void SetItem(const DragPayload& p, Onyx::Texture* tex,
					 glm::vec2 uvMin = {0.0f, 0.0f},
					 glm::vec2 uvMax = {1.0f, 1.0f});
		void Clear();
		bool IsEmpty() const               { return m_Empty; }
		const DragPayload& GetPayload() const { return m_Payload; }

		// Drop gating + handler. Default accepts anything.
		void SetAcceptFilter(std::function<bool(const DragPayload&)> fn)
		{ m_Accept = std::move(fn); }
		void SetOnDrop(std::function<void(const DragPayload&, uint16_t mods)> fn)
		{ m_OnDrop = std::move(fn); }

		bool Accepts(const DragPayload& p) const;
		void ReceiveDrop(const DragPayload& p, uint16_t mods);

		void SetSlotColors(const Color& bg, const Color& border,
						   const Color& highlight)
		{ m_Bg = bg; m_Border = border; m_Highlight = highlight; }
		void SetCornerRadius(float r) { m_Radius = r; }

		void OnInput(const InputEvent& e) override;
		void OnDetach() override;
		void OnDraw(UIRenderer& r) override;

	private:
		bool IsDragSource() const;
		bool IsHotTarget() const; // compatible drag hovering this slot

		DragContext* m_Ctx = nullptr;

		bool        m_Empty = true;
		DragPayload m_Payload;
		Onyx::Texture* m_Tex = nullptr;
		glm::vec2 m_UvMin{0.0f, 0.0f};
		glm::vec2 m_UvMax{1.0f, 1.0f};

		std::function<bool(const DragPayload&)> m_Accept;
		std::function<void(const DragPayload&, uint16_t)> m_OnDrop;

		Color m_Bg        = Color::FromRGBA8(0x1A1F29E0);
		Color m_Border    = Color::FromRGBA8(0x000000B0);
		Color m_Highlight = Color::FromRGBA8(0xFFD24AFF);
		float m_Radius = 4.0f;
	};

} // namespace Onyx::UI
