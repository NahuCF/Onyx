#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace Onyx::UI {

	// How a single dimension (width or height) is computed when the parent
	// performs layout. Each axis is configured independently per widget.
	enum class SizeMode : uint8_t
	{
		Fixed	= 0, // absolute pixels (the value field)
		Fill	= 1, // consume all available space along this axis (after siblings)
		Percent = 2, // fraction of parent's content width/height (0..1 in value)
		Aspect	= 3, // derived from the other axis × value (e.g. width = height * 1.5)
	};

	// Placement within a Free-layout parent. Used together with `anchorOffset`
	// to position the widget relative to the parent's content area.
	enum class AnchorEdge : uint8_t
	{
		TopLeft = 0,
		Top,
		TopRight,
		Left,
		Center,
		Right,
		BottomLeft,
		Bottom,
		BottomRight,
	};

	// How a parent arranges its children. Free leaves placement to each child's
	// anchor + offset; Stack lays children out sequentially with gap.
	enum class ContainerMode : uint8_t
	{
		Free			= 0, // children placed by their own anchor + offset
		StackHorizontal = 1,
		StackVertical	= 2,
	};

	// Per-widget layout configuration. Default constructs to a 100×100 fixed
	// box, anchored top-left at (0, 0), no padding, no flex. Containers default
	// to Free mode.
	struct LayoutSpec
	{
		SizeMode widthMode = SizeMode::Fixed;
		float width = 100.0f;
		SizeMode heightMode = SizeMode::Fixed;
		float height = 100.0f;

		// Constraints applied after primary sizing. Min wins over max, max
		// wins over the computed value.
		glm::vec2 minSize{0.0f};
		glm::vec2 maxSize{1e9f};

		// Used in Free containers.
		AnchorEdge anchor = AnchorEdge::TopLeft;
		glm::vec2 anchorOffset{0.0f};

		// Stack container: positive values share the parent's free space along
		// its axis after fixed-size siblings are placed. flexGrow=0 means the
		// widget keeps its preferred size.
		float flexGrow = 0.0f;

		// Inset applied to this widget's bounds when laying out its own
		// children. {top, right, bottom, left}.
		glm::vec4 padding{0.0f};

		// Stack containers only: pixels of empty space between children.
		float gap = 0.0f;
	};

} // namespace Onyx::UI
