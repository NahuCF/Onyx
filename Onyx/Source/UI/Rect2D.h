#pragma once

#include <algorithm>
#include <glm/glm.hpp>

namespace Onyx::UI {

	// Axis-aligned 2D rectangle in pixels, top-left origin (Y down to match ImGui
	// and the rest of the screen-space stack). Min is inclusive, Max is exclusive
	// for hit-testing — a rect with min=(0,0) max=(10,10) covers pixels 0..9.
	struct Rect2D
	{
		glm::vec2 min{0.0f};
		glm::vec2 max{0.0f};

		Rect2D() = default;
		constexpr Rect2D(glm::vec2 mn, glm::vec2 mx) : min(mn), max(mx) {}

		static Rect2D FromXYWH(float x, float y, float w, float h)
		{
			return Rect2D{glm::vec2(x, y), glm::vec2(x + w, y + h)};
		}

		float Width() const { return max.x - min.x; }
		float Height() const { return max.y - min.y; }
		glm::vec2 Size() const { return max - min; }
		glm::vec2 Center() const { return (min + max) * 0.5f; }

		bool Empty() const { return max.x <= min.x || max.y <= min.y; }

		bool Contains(glm::vec2 p) const
		{
			return p.x >= min.x && p.x < max.x && p.y >= min.y && p.y < max.y;
		}

		bool Intersects(const Rect2D& o) const
		{
			return !(o.max.x <= min.x || o.min.x >= max.x ||
					 o.max.y <= min.y || o.min.y >= max.y);
		}

		// Intersection. Returns an empty rect if disjoint.
		Rect2D Intersected(const Rect2D& o) const
		{
			Rect2D r;
			r.min.x = std::max(min.x, o.min.x);
			r.min.y = std::max(min.y, o.min.y);
			r.max.x = std::min(max.x, o.max.x);
			r.max.y = std::min(max.y, o.max.y);
			if (r.max.x < r.min.x) r.max.x = r.min.x;
			if (r.max.y < r.min.y) r.max.y = r.min.y;
			return r;
		}

		// Smallest rect containing both.
		Rect2D Unioned(const Rect2D& o) const
		{
			if (Empty()) return o;
			if (o.Empty()) return *this;
			Rect2D r;
			r.min.x = std::min(min.x, o.min.x);
			r.min.y = std::min(min.y, o.min.y);
			r.max.x = std::max(max.x, o.max.x);
			r.max.y = std::max(max.y, o.max.y);
			return r;
		}

		// Inset all four edges by delta. Negative delta grows outward.
		Rect2D Inset(float delta) const
		{
			return Rect2D{min + glm::vec2(delta), max - glm::vec2(delta)};
		}

		// Asymmetric insets (top, right, bottom, left). Negative grows outward.
		Rect2D Inset(float top, float right, float bottom, float left) const
		{
			return Rect2D{glm::vec2(min.x + left, min.y + top),
						  glm::vec2(max.x - right, max.y - bottom)};
		}

		Rect2D Translate(glm::vec2 d) const { return Rect2D{min + d, max + d}; }

		bool operator==(const Rect2D& o) const noexcept { return min == o.min && max == o.max; }
	};

} // namespace Onyx::UI
