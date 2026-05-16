#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Onyx { class Texture; }

namespace Onyx::UI {

	class DragSlot;

	// Generic drag payload. `type` gates which DropTargets accept it
	// ("item", "spell", ...); id/count/user carry the game meaning.
	struct DragPayload
	{
		std::string type;
		int64_t     id    = 0;
		int         count = 1;
		void*       user  = nullptr;
	};

	// One active drag at a time. Owns the payload + ghost visual + the slot
	// registry, and resolves a drop against whichever registered slot is under
	// the cursor. Built on the drag events WidgetTree already synthesizes
	// (threshold detection lives there) — this is the M8 semantic layer.
	//
	// v1 scope: drag-from/drop-onto slots, ghost follow, modifier-aware drop,
	// drop-outside = snap-back/cancel. DragGhost override, :drop-reject
	// pseudo-state and Escape-mid-drag-cancel are documented M8 follow-ups.
	class DragContext
	{
	public:
		void Begin(DragSlot* source, const DragPayload& payload,
				   Onyx::Texture* ghostTex, glm::vec2 ghostUvMin,
				   glm::vec2 ghostUvMax, glm::vec2 ghostSize);

		void UpdateCursor(glm::vec2 screenPos) { m_Cursor = screenPos; }

		// End the drag. Drop() delivers to a slot under the cursor (if any
		// accepts); Cancel() / no-target = snap-back (payload not delivered).
		void Drop(uint16_t modifiers);
		void Cancel();

		bool IsDragging() const               { return m_Active; }
		const DragPayload& Payload() const     { return m_Payload; }
		glm::vec2 Cursor() const               { return m_Cursor; }
		DragSlot* Source() const               { return m_Source; }

		Onyx::Texture* GhostTexture() const    { return m_GhostTex; }
		glm::vec2 GhostUvMin() const           { return m_GhostUvMin; }
		glm::vec2 GhostUvMax() const           { return m_GhostUvMax; }
		glm::vec2 GhostSize() const            { return m_GhostSize; }

		// Slots self-register so Drop() can hit-test them without walking the
		// whole tree.
		void RegisterSlot(DragSlot* s);
		void UnregisterSlot(DragSlot* s);

	private:
		bool        m_Active = false;
		DragSlot*   m_Source = nullptr;
		DragPayload m_Payload;
		glm::vec2   m_Cursor{0.0f};

		Onyx::Texture* m_GhostTex = nullptr;
		glm::vec2 m_GhostUvMin{0.0f};
		glm::vec2 m_GhostUvMax{1.0f};
		glm::vec2 m_GhostSize{40.0f};

		std::vector<DragSlot*> m_Slots;
	};

} // namespace Onyx::UI
