#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace Onyx::UI {

	// Modifier flags carried on every input event. Combined with bitwise OR.
	// Gamepad bits are reserved (Q6 — gamepad support is out of scope but the
	// struct is shaped to accept it).
	enum class ModifierFlag : uint16_t
	{
		None  = 0,
		Shift = 1 << 0,
		Ctrl  = 1 << 1,
		Alt   = 1 << 2,
		Super = 1 << 3, // Win/Cmd/Meta
		// Reserved: Gamepad button states would slot in here at 1<<8 onward.
	};

	using ModifierFlags = uint16_t;

	inline ModifierFlags operator|(ModifierFlag a, ModifierFlag b)
	{
		return static_cast<ModifierFlags>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
	}

	inline bool HasMod(ModifierFlags flags, ModifierFlag m)
	{
		return (flags & static_cast<uint16_t>(m)) != 0;
	}

	enum class MouseButton : uint8_t
	{
		Left   = 0,
		Right  = 1,
		Middle = 2,
		X1	   = 3,
		X2	   = 4,
		Count
	};

	enum class InputEventType : uint8_t
	{
		MouseMove = 0,
		MouseDown,
		MouseUp,
		MouseWheel,
		KeyDown,
		KeyUp,
		Char,			 // text input (post-IME if/when added)
		FocusEnter,
		FocusLeave,
		// Synthesized by the framework, not the platform layer:
		MouseDoubleClick,
		MouseLongPress,
		MouseDragBegin,
		MouseDrag,
		MouseDragEnd,
		HoverEnter,		 // pointer entered widget bounds
		HoverLeave,		 // pointer left widget bounds
		HoverIntent,	 // pointer dwelt past the hover-intent threshold
	};

	// Single discriminated event. Position is in screen pixels (top-left origin).
	// Widget-local coordinates are derived during dispatch.
	struct InputEvent
	{
		InputEventType type = InputEventType::MouseMove;
		ModifierFlags  mods = 0;

		glm::vec2 position{0.0f}; // screen-space pixel coordinates
		glm::vec2 delta{0.0f};	  // mouse: motion since last move; wheel: scroll {x,y}

		MouseButton button = MouseButton::Left; // for MouseDown/Up/DoubleClick/LongPress/Drag*
		uint8_t	 clickCount = 1;				// 1=single, 2=double; up to 250 ms gap

		uint32_t keyCode = 0;	   // GLFW key code for KeyDown/KeyUp
		uint32_t scanCode = 0;	   // platform scancode (passthrough)
		uint32_t character = 0;	   // unicode codepoint for Char events
		bool	 repeat = false;	// auto-repeat held key

		// Set true by a handler to stop further dispatch within the current
		// dispatch pass. Equivalent to event.preventDefault + stopPropagation.
		mutable bool handled = false;
	};

	// Centralized default thresholds. Theme can override per-widget; these are
	// the engine-wide defaults documented in the design doc.
	struct InputThresholds
	{
		float dragPx			   = 4.0f;	// pixels of movement before a mouse-down becomes a drag
		float doubleClickSeconds   = 0.25f; // max gap for a click pair to count as double-click
		float longPressSeconds	   = 0.6f;	// hold time before MouseLongPress fires
		float hoverIntentSeconds   = 0.4f;	// pointer dwell before HoverIntent fires
	};

} // namespace Onyx::UI
