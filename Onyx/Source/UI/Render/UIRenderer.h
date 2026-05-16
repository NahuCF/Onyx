#pragma once

#include "Source/UI/Color.h"
#include "Source/UI/Rect2D.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <string_view>
#include <vector>

namespace Onyx {
	class VertexArray;
	class VertexBuffer;
	class IndexBuffer;
	class Texture;
} // namespace Onyx

namespace Onyx::UI {

	class FontAtlas;

	struct UIVertex
	{
		glm::vec2 pos;	  // screen-space pixels (top-left origin)
		glm::vec2 uv;	  // 0..1 within current texture
		glm::vec4 color;  // premultiplied RGBA
	};

	// Single batched 2D renderer for the UI library. Premultiplied alpha
	// throughout. Forces a flush on texture change or scissor change; gradient,
	// rounded, and 9-slice paths add geometry to the same batch when the
	// texture (typically the white pixel) is unchanged.
	//
	// Performance commitments per the design doc: batched HUD in 1-5 draws/frame.
	// State changes are amortized within batch boundaries.
	class UIRenderer
	{
	public:
		struct Stats
		{
			uint32_t drawCalls = 0;
			uint32_t quads = 0;
			uint32_t batchBreaks = 0;
			uint32_t scissorChanges = 0;
			uint32_t maskBinds = 0;
		};

		UIRenderer();
		~UIRenderer();

		void Init();
		void Shutdown();

		// Per-frame setup. Sets the orthographic projection (top-left origin,
		// pixel coordinates) and resets stats.
		void BeginFrame(glm::ivec2 viewport);
		void EndFrame();

		// Solid-colored rect.
		void DrawRect(const Rect2D& bounds, const Color& color);

		// Four-corner gradient. Colors at TL/TR/BL/BR; bilinear interpolation
		// across the rect.
		void DrawGradient(const Rect2D& bounds, const Color& tl, const Color& tr,
						  const Color& bl, const Color& br);

		// Solid-colored rect with rounded corners (corner-fan tessellation).
		// Fragment SDF rounding is a future optimization (M2 deferred).
		void DrawRoundedRect(const Rect2D& bounds, const Color& color, float radius);

		// Filled pie wedge (triangle fan from center). Angles in radians,
		// 0 = +X, increasing clockwise on screen (Y-down). Used for the
		// action-bar cooldown sweep. segments<=0 auto-scales with radius.
		void DrawPie(glm::vec2 center, float radius, float startRad,
					 float sweepRad, const Color& color, int segments = 0);

		// Textured quad. Pass nullptr to use the white-pixel texture (same as
		// DrawRect).
		void DrawTexture(const Rect2D& bounds, Onyx::Texture* tex, const Color& tint = Color::White());
		void DrawTextureRegion(const Rect2D& bounds, Onyx::Texture* tex,
							   glm::vec2 uvMin, glm::vec2 uvMax,
							   const Color& tint = Color::White());

		// 9-slice resizable image. `borderPx` is {top, right, bottom, left}
		// border thickness in source-texture pixels. Texture provides the
		// nine source regions; bounds receives the resized output.
		void Draw9Slice(const Rect2D& bounds, Onyx::Texture* tex, glm::vec4 borderPx,
						const Color& tint = Color::White());

		enum class TextAlignH : uint8_t { Left, Center, Right };

		// Render UTF-8 (latin subset) text using an MSDF font atlas. The pen
		// starts at `bounds.min` (top-left); single-line. `pxSize` is the
		// runtime display height; the shader scales the atlas's bake-time
		// distance field accordingly. Returns the advance pen position so
		// callers can chain runs.
		glm::vec2 DrawText(const Rect2D& bounds, std::string_view text,
						   const FontAtlas& font, float pxSize,
						   const Color& color,
						   TextAlignH align = TextAlignH::Left);

		// Compute the bounding box of a text run with the given font + size.
		// Returns (width, height) in pixels. Used by Label for content-driven
		// sizing.
		glm::vec2 MeasureText(std::string_view text, const FontAtlas& font, float pxSize);

		// Scissor stack. Push intersects with the parent scissor; depth is
		// limited to MaxScissorDepth (warns and ignores beyond). Pop returns
		// to the previous scissor (or unscissored if stack empties).
		static constexpr int MaxScissorDepth = 16;
		void PushScissor(const Rect2D& clip);
		void PopScissor();

		const Stats& GetStats() const { return m_Stats; }

	private:
		enum class BatchShader : uint8_t { Default, Text };

		void Flush();
		void EnsureCapacity(uint32_t addQuads);
		void SetBatchTexture(Onyx::Texture* tex);
		void SetBatchShader(BatchShader shader);
		void ApplyScissor();

		uint32_t AppendQuadVertices(const Rect2D& bounds, const Color (&corners)[4],
									const glm::vec2& uvMin, const glm::vec2& uvMax);

		// Whether two scissor rects are equal enough to skip a state change.
		static bool ScissorsEqual(const Rect2D& a, const Rect2D& b);

		// GL state
		uint32_t m_Program = 0;	  // default UI shader (solid + textured + gradient)
		int m_LocProj = -1;
		int m_LocTex0 = -1;

		uint32_t m_TextProgram = 0;  // MSDF text shader
		int m_TextLocProj = -1;
		int m_TextLocTex0 = -1;
		int m_TextLocPxRange = -1;
		float m_BatchPxRange = 0.0f; // most-recently-set pxRange for the text batch

		std::unique_ptr<Onyx::VertexArray>	 m_VAO;
		std::unique_ptr<Onyx::VertexBuffer>	 m_VBO;
		std::unique_ptr<Onyx::IndexBuffer>	 m_IBO;
		std::unique_ptr<Onyx::Texture>		 m_WhitePixel;

		std::vector<UIVertex>  m_Vertices;
		std::vector<uint32_t>  m_Indices;
		uint32_t m_VertexCapacity = 0;
		uint32_t m_IndexCapacity = 0;

		// Current batch state
		Onyx::Texture* m_BatchTexture = nullptr;
		BatchShader m_BatchShader = BatchShader::Default;
		Rect2D m_CurrentScissor;
		bool m_ScissorActive = false;

		// Scissor stack (depth limited)
		struct ScissorEntry
		{
			Rect2D rect;
			bool active = false;
		};
		ScissorEntry m_ScissorStack[MaxScissorDepth];
		int m_ScissorDepth = 0;

		glm::ivec2 m_Viewport{0, 0};
		glm::mat4  m_OrthoProjection{1.0f};

		Stats m_Stats;
		bool  m_FrameActive = false;
	};

} // namespace Onyx::UI
