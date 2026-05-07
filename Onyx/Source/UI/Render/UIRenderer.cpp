#include "UIRenderer.h"

#include "Source/Graphics/Buffers.h"
#include "Source/Graphics/Texture.h"
#include "Source/Graphics/VertexLayout.h"
#include "Source/UI/Text/FontAtlas.h"

#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace Onyx::UI {

	namespace {

		constexpr uint32_t kMaxQuads = 64 * 1024; // ~64k quads/frame headroom
		constexpr uint32_t kVertsPerQuad = 4;
		constexpr uint32_t kIndicesPerQuad = 6;

		const char* kVertSrc = R"(#version 410 core
layout(location = 0) in vec2 a_Pos;
layout(location = 1) in vec2 a_UV;
layout(location = 2) in vec4 a_Color;

uniform mat4 u_Proj;

out vec2 v_UV;
out vec4 v_Color;

void main() {
    gl_Position = u_Proj * vec4(a_Pos, 0.0, 1.0);
    v_UV = a_UV;
    v_Color = a_Color;
}
)";

		const char* kFragSrc = R"(#version 410 core
in vec2 v_UV;
in vec4 v_Color;

uniform sampler2D u_Tex0;

out vec4 FragColor;

void main() {
    // Both texture and color are premultiplied. Multiplying premultiplied
    // values in this shader yields a premultiplied result; the framebuffer
    // blend mode (GL_ONE, GL_ONE_MINUS_SRC_ALPHA) does the rest.
    vec4 t = texture(u_Tex0, v_UV);
    FragColor = t * v_Color;
}
)";

		// MSDF text fragment shader. Samples the RGB MSDF atlas, takes the
		// median of the three channels (the multi-channel SDF), and converts
		// to coverage based on the current screen-pixel range. u_PxRange is
		// the bake-time pixelRange scaled by (displaySize / atlasBakeSize).
		const char* kTextFragSrc = R"(#version 410 core
in vec2 v_UV;
in vec4 v_Color;

uniform sampler2D u_Tex0;
uniform float u_PxRange;

out vec4 FragColor;

float median(float a, float b, float c) {
    return max(min(a, b), min(max(a, b), c));
}

void main() {
    vec3 msd = texture(u_Tex0, v_UV).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    // sd is in 0..1 with 0.5 = boundary; scale to screen-pixel signed distance.
    float screenPxDist = u_PxRange * (sd - 0.5);
    float coverage = clamp(screenPxDist + 0.5, 0.0, 1.0);
    // v_Color already premultiplied by Color::Premultiplied at vertex emit.
    FragColor = v_Color * coverage;
}
)";

		uint32_t CompileShader(uint32_t kind, const char* src, const char* tag)
		{
			uint32_t id = glCreateShader(kind);
			glShaderSource(id, 1, &src, nullptr);
			glCompileShader(id);

			GLint ok = GL_FALSE;
			glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
			if (!ok)
			{
				char log[2048] = {};
				GLsizei len = 0;
				glGetShaderInfoLog(id, sizeof(log), &len, log);
				std::cerr << "[UI] shader compile failed (" << tag << "): " << log << '\n';
				glDeleteShader(id);
				return 0;
			}
			return id;
		}

	} // namespace

	UIRenderer::UIRenderer() = default;
	UIRenderer::~UIRenderer() { Shutdown(); }

	void UIRenderer::Init()
	{
		auto link = [](uint32_t vs, uint32_t fs, const char* tag) -> uint32_t {
			uint32_t program = glCreateProgram();
			glAttachShader(program, vs);
			glAttachShader(program, fs);
			glLinkProgram(program);
			GLint linked = GL_FALSE;
			glGetProgramiv(program, GL_LINK_STATUS, &linked);
			if (!linked)
			{
				char log[2048] = {};
				GLsizei len = 0;
				glGetProgramInfoLog(program, sizeof(log), &len, log);
				std::cerr << "[UI] " << tag << " link failed: " << log << '\n';
				glDeleteProgram(program);
				return 0;
			}
			return program;
		};

		// Default UI program
		uint32_t vs = CompileShader(GL_VERTEX_SHADER, kVertSrc, "ui.vert");
		uint32_t fs = CompileShader(GL_FRAGMENT_SHADER, kFragSrc, "ui.frag");
		uint32_t fsText = CompileShader(GL_FRAGMENT_SHADER, kTextFragSrc, "ui-text.frag");
		if (!vs || !fs || !fsText)
		{
			if (vs) glDeleteShader(vs);
			if (fs) glDeleteShader(fs);
			if (fsText) glDeleteShader(fsText);
			std::cerr << "[UI] renderer shader build aborted\n";
			return;
		}

		m_Program = link(vs, fs, "ui");
		m_TextProgram = link(vs, fsText, "ui-text");

		glDeleteShader(vs);
		glDeleteShader(fs);
		glDeleteShader(fsText);

		if (m_Program)
		{
			m_LocProj = glGetUniformLocation(m_Program, "u_Proj");
			m_LocTex0 = glGetUniformLocation(m_Program, "u_Tex0");
		}
		if (m_TextProgram)
		{
			m_TextLocProj    = glGetUniformLocation(m_TextProgram, "u_Proj");
			m_TextLocTex0    = glGetUniformLocation(m_TextProgram, "u_Tex0");
			m_TextLocPxRange = glGetUniformLocation(m_TextProgram, "u_PxRange");
		}

		// Pre-allocate the maximum vertex/index buffers so we never reallocate
		// mid-frame. Pool allocator is overkill at this scale.
		m_VertexCapacity = kMaxQuads * kVertsPerQuad;
		m_IndexCapacity  = kMaxQuads * kIndicesPerQuad;
		m_Vertices.reserve(m_VertexCapacity);
		m_Indices.reserve(m_IndexCapacity);

		m_VAO = std::make_unique<Onyx::VertexArray>();
		m_VAO->Bind();

		m_VBO = std::make_unique<Onyx::VertexBuffer>(static_cast<uint32_t>(m_VertexCapacity * sizeof(UIVertex)));
		m_IBO = std::make_unique<Onyx::IndexBuffer>(static_cast<uint32_t>(m_IndexCapacity * sizeof(uint32_t)));

		Onyx::VertexLayout layout;
		layout.PushFloat(2); // pos
		layout.PushFloat(2); // uv
		layout.PushFloat(4); // color

		m_VAO->SetVertexBuffer(m_VBO.get());
		m_VAO->SetIndexBuffer(m_IBO.get());
		m_VAO->SetLayout(layout);

		// 1x1 white pixel for solid-colored quads. The shader stays uniform
		// (always samples a texture) which keeps the batch simple.
		m_WhitePixel = Onyx::Texture::CreateSolidColor(255, 255, 255, 255);
	}

	void UIRenderer::Shutdown()
	{
		if (m_Program)     { glDeleteProgram(m_Program);     m_Program = 0; }
		if (m_TextProgram) { glDeleteProgram(m_TextProgram); m_TextProgram = 0; }
		m_VAO.reset();
		m_VBO.reset();
		m_IBO.reset();
		m_WhitePixel.reset();
		m_Vertices.clear();
		m_Indices.clear();
	}

	void UIRenderer::BeginFrame(glm::ivec2 viewport)
	{
		m_Viewport = viewport;
		// Top-left origin, Y down. Matches ImGui and the rest of the screen-
		// space stack. Pixel coordinates as input.
		m_OrthoProjection = glm::ortho(0.0f,
									   static_cast<float>(viewport.x),
									   static_cast<float>(viewport.y),
									   0.0f,
									   -1.0f, 1.0f);
		m_Stats = {};
		m_BatchTexture = nullptr;
		m_BatchShader = BatchShader::Default;
		m_ScissorActive = false;
		m_ScissorDepth = 0;
		m_FrameActive = true;
	}

	void UIRenderer::EndFrame()
	{
		Flush();
		if (m_ScissorActive)
		{
			glDisable(GL_SCISSOR_TEST);
			m_ScissorActive = false;
		}
		m_FrameActive = false;
	}

	void UIRenderer::EnsureCapacity(uint32_t addQuads)
	{
		const uint32_t needV = static_cast<uint32_t>(m_Vertices.size()) + addQuads * kVertsPerQuad;
		if (needV > m_VertexCapacity)
			Flush();
	}

	void UIRenderer::SetBatchTexture(Onyx::Texture* tex)
	{
		Onyx::Texture* effective = tex ? tex : m_WhitePixel.get();
		if (effective != m_BatchTexture)
		{
			Flush();
			m_BatchTexture = effective;
		}
	}

	void UIRenderer::SetBatchShader(BatchShader shader)
	{
		if (shader != m_BatchShader)
		{
			Flush();
			m_BatchShader = shader;
		}
	}

	bool UIRenderer::ScissorsEqual(const Rect2D& a, const Rect2D& b)
	{
		return a.min == b.min && a.max == b.max;
	}

	void UIRenderer::ApplyScissor()
	{
		// glScissor uses bottom-left origin, integer pixels.
		const Rect2D r = m_CurrentScissor;
		const int x = static_cast<int>(std::floor(r.min.x));
		const int yTop = static_cast<int>(std::floor(r.min.y));
		const int w = std::max(0, static_cast<int>(std::ceil(r.max.x)) - x);
		const int h = std::max(0, static_cast<int>(std::ceil(r.max.y)) - yTop);
		const int yBL = m_Viewport.y - (yTop + h);
		glScissor(x, yBL, w, h);
	}

	void UIRenderer::PushScissor(const Rect2D& clip)
	{
		Flush();

		Rect2D effective = clip;
		if (m_ScissorActive)
			effective = effective.Intersected(m_CurrentScissor);

		if (m_ScissorDepth >= MaxScissorDepth)
		{
			std::cerr << "[UI] scissor stack overflow (max " << MaxScissorDepth << ") — clip ignored\n";
			return;
		}

		m_ScissorStack[m_ScissorDepth].rect = m_CurrentScissor;
		m_ScissorStack[m_ScissorDepth].active = m_ScissorActive;
		++m_ScissorDepth;

		m_CurrentScissor = effective;
		if (!m_ScissorActive)
		{
			glEnable(GL_SCISSOR_TEST);
			m_ScissorActive = true;
		}
		ApplyScissor();
		++m_Stats.scissorChanges;
	}

	void UIRenderer::PopScissor()
	{
		if (m_ScissorDepth <= 0)
			return;

		Flush();
		--m_ScissorDepth;
		const ScissorEntry& prev = m_ScissorStack[m_ScissorDepth];
		m_CurrentScissor = prev.rect;
		if (prev.active)
		{
			if (!m_ScissorActive)
			{
				glEnable(GL_SCISSOR_TEST);
				m_ScissorActive = true;
			}
			ApplyScissor();
		}
		else if (m_ScissorActive)
		{
			glDisable(GL_SCISSOR_TEST);
			m_ScissorActive = false;
		}
		++m_Stats.scissorChanges;
	}

	uint32_t UIRenderer::AppendQuadVertices(const Rect2D& bounds, const Color (&corners)[4],
											const glm::vec2& uvMin, const glm::vec2& uvMax)
	{
		const uint32_t base = static_cast<uint32_t>(m_Vertices.size());

		// Premultiply at emit time so the shader can stay uniform.
		auto premul = [](const Color& c) {
			return glm::vec4(c.r * c.a, c.g * c.a, c.b * c.a, c.a);
		};

		// Order: TL, TR, BL, BR
		m_Vertices.push_back({glm::vec2(bounds.min.x, bounds.min.y), uvMin, premul(corners[0])});
		m_Vertices.push_back({glm::vec2(bounds.max.x, bounds.min.y),
							  glm::vec2(uvMax.x, uvMin.y), premul(corners[1])});
		m_Vertices.push_back({glm::vec2(bounds.min.x, bounds.max.y),
							  glm::vec2(uvMin.x, uvMax.y), premul(corners[2])});
		m_Vertices.push_back({glm::vec2(bounds.max.x, bounds.max.y), uvMax, premul(corners[3])});

		// Two triangles: TL-TR-BL, TR-BR-BL
		m_Indices.push_back(base + 0);
		m_Indices.push_back(base + 1);
		m_Indices.push_back(base + 2);
		m_Indices.push_back(base + 1);
		m_Indices.push_back(base + 3);
		m_Indices.push_back(base + 2);

		++m_Stats.quads;
		return base;
	}

	void UIRenderer::DrawRect(const Rect2D& bounds, const Color& color)
	{
		if (bounds.Empty()) return;
		EnsureCapacity(1);
		SetBatchShader(BatchShader::Default);
		SetBatchTexture(nullptr);
		const Color cs[4] = {color, color, color, color};
		AppendQuadVertices(bounds, cs, glm::vec2(0.0f), glm::vec2(1.0f));
	}

	void UIRenderer::DrawGradient(const Rect2D& bounds, const Color& tl, const Color& tr,
								  const Color& bl, const Color& br)
	{
		if (bounds.Empty()) return;
		EnsureCapacity(1);
		SetBatchShader(BatchShader::Default);
		SetBatchTexture(nullptr);
		const Color cs[4] = {tl, tr, bl, br};
		AppendQuadVertices(bounds, cs, glm::vec2(0.0f), glm::vec2(1.0f));
	}

	void UIRenderer::DrawTexture(const Rect2D& bounds, Onyx::Texture* tex, const Color& tint)
	{
		if (bounds.Empty()) return;
		EnsureCapacity(1);
		SetBatchShader(BatchShader::Default);
		SetBatchTexture(tex);
		const Color cs[4] = {tint, tint, tint, tint};
		AppendQuadVertices(bounds, cs, glm::vec2(0.0f), glm::vec2(1.0f));
	}

	void UIRenderer::DrawTextureRegion(const Rect2D& bounds, Onyx::Texture* tex,
									   glm::vec2 uvMin, glm::vec2 uvMax, const Color& tint)
	{
		if (bounds.Empty()) return;
		EnsureCapacity(1);
		SetBatchShader(BatchShader::Default);
		SetBatchTexture(tex);
		const Color cs[4] = {tint, tint, tint, tint};
		AppendQuadVertices(bounds, cs, uvMin, uvMax);
	}

	void UIRenderer::DrawRoundedRect(const Rect2D& bounds, const Color& color, float radius)
	{
		if (bounds.Empty()) return;

		const float w = bounds.Width();
		const float h = bounds.Height();
		radius = std::min(radius, std::min(w, h) * 0.5f);

		if (radius <= 0.5f)
		{
			DrawRect(bounds, color);
			return;
		}

		// Decompose into:
		//   center rect
		//   4 edge rects (top / right / bottom / left, each minus the corners)
		//   4 corner fans (each an n-segment triangle fan)
		// Total geometry: 1 + 4 = 5 quads + 4 fans.
		// For M2 we emit fans as triangles via index buffer; vertices share
		// the same UIVertex layout (UVs all zero, white texture).
		//
		// All these go into the same batch (white pixel texture, premul color),
		// so only one Flush at the end.
		SetBatchShader(BatchShader::Default);
		SetBatchTexture(nullptr);

		const Rect2D center = Rect2D::FromXYWH(bounds.min.x + radius, bounds.min.y + radius,
											   w - 2.0f * radius, h - 2.0f * radius);
		const Rect2D top	= Rect2D::FromXYWH(center.min.x, bounds.min.y, center.Width(), radius);
		const Rect2D bottom = Rect2D::FromXYWH(center.min.x, center.max.y, center.Width(), radius);
		const Rect2D left	= Rect2D::FromXYWH(bounds.min.x, center.min.y, radius, center.Height());
		const Rect2D right	= Rect2D::FromXYWH(center.max.x, center.min.y, radius, center.Height());

		EnsureCapacity(5);
		const Color cs[4] = {color, color, color, color};
		AppendQuadVertices(center, cs, glm::vec2(0.0f), glm::vec2(1.0f));
		AppendQuadVertices(top,	   cs, glm::vec2(0.0f), glm::vec2(1.0f));
		AppendQuadVertices(bottom, cs, glm::vec2(0.0f), glm::vec2(1.0f));
		AppendQuadVertices(left,   cs, glm::vec2(0.0f), glm::vec2(1.0f));
		AppendQuadVertices(right,  cs, glm::vec2(0.0f), glm::vec2(1.0f));

		// Corner fans. Segment count scales with radius.
		const int segs = std::max(4, std::min(16, static_cast<int>(radius * 0.5f)));
		const glm::vec4 premulColor(color.r * color.a, color.g * color.a, color.b * color.a, color.a);

		auto fan = [&](glm::vec2 cornerCenter, float startAngleRad)
		{
			// Need 1 center vertex + (segs+1) ring vertices + segs triangles.
			const uint32_t base = static_cast<uint32_t>(m_Vertices.size());
			m_Vertices.push_back({cornerCenter, glm::vec2(0.0f), premulColor});
			for (int i = 0; i <= segs; ++i)
			{
				const float t = static_cast<float>(i) / static_cast<float>(segs);
				const float a = startAngleRad + t * (3.14159265f * 0.5f);
				const glm::vec2 p = cornerCenter + glm::vec2(std::cos(a), std::sin(a)) * radius;
				m_Vertices.push_back({p, glm::vec2(0.0f), premulColor});
			}
			for (int i = 0; i < segs; ++i)
			{
				m_Indices.push_back(base);
				m_Indices.push_back(base + 1 + i);
				m_Indices.push_back(base + 2 + i);
			}
			++m_Stats.quads; // count fans as one logical quad for stats
		};

		// Corners: TL, TR, BL, BR. Y goes down, so angles are CCW in the
		// y-down frame (sign of sin flips). We pick angles such that each
		// fan covers its corner outward.
		fan(glm::vec2(bounds.min.x + radius, bounds.min.y + radius), 3.14159265f);            // TL: 180° → 270°
		fan(glm::vec2(bounds.max.x - radius, bounds.min.y + radius), 3.14159265f * 1.5f);     // TR: 270° → 360°
		fan(glm::vec2(bounds.min.x + radius, bounds.max.y - radius), 3.14159265f * 0.5f);     // BL: 90° → 180°
		fan(glm::vec2(bounds.max.x - radius, bounds.max.y - radius), 0.0f);                   // BR: 0° → 90°
	}

	void UIRenderer::Draw9Slice(const Rect2D& bounds, Onyx::Texture* tex, glm::vec4 borderPx,
								const Color& tint)
	{
		if (!tex || bounds.Empty())
			return;

		const float t = borderPx.x;
		const float r = borderPx.y;
		const float b = borderPx.z;
		const float l = borderPx.w;

		const auto sz = tex->GetTextureSize();
		const float texW = static_cast<float>(sz.x);
		const float texH = static_cast<float>(sz.y);
		if (texW <= 0.0f || texH <= 0.0f) return;

		// UV ranges per row/column
		const float u0 = 0.0f;
		const float u1 = l / texW;
		const float u2 = 1.0f - r / texW;
		const float u3 = 1.0f;
		const float v0 = 0.0f;
		const float v1 = t / texH;
		const float v2 = 1.0f - b / texH;
		const float v3 = 1.0f;

		// Pixel coordinates
		const float x0 = bounds.min.x;
		const float x1 = bounds.min.x + l;
		const float x2 = bounds.max.x - r;
		const float x3 = bounds.max.x;
		const float y0 = bounds.min.y;
		const float y1 = bounds.min.y + t;
		const float y2 = bounds.max.y - b;
		const float y3 = bounds.max.y;

		struct Cell
		{
			Rect2D rect;
			glm::vec2 uvMin;
			glm::vec2 uvMax;
		};
		const Cell cells[9] = {
			{Rect2D{{x0, y0}, {x1, y1}}, {u0, v0}, {u1, v1}},
			{Rect2D{{x1, y0}, {x2, y1}}, {u1, v0}, {u2, v1}},
			{Rect2D{{x2, y0}, {x3, y1}}, {u2, v0}, {u3, v1}},
			{Rect2D{{x0, y1}, {x1, y2}}, {u0, v1}, {u1, v2}},
			{Rect2D{{x1, y1}, {x2, y2}}, {u1, v1}, {u2, v2}},
			{Rect2D{{x2, y1}, {x3, y2}}, {u2, v1}, {u3, v2}},
			{Rect2D{{x0, y2}, {x1, y3}}, {u0, v2}, {u1, v3}},
			{Rect2D{{x1, y2}, {x2, y3}}, {u1, v2}, {u2, v3}},
			{Rect2D{{x2, y2}, {x3, y3}}, {u2, v2}, {u3, v3}},
		};

		EnsureCapacity(9);
		SetBatchShader(BatchShader::Default);
		SetBatchTexture(tex);
		const Color cs[4] = {tint, tint, tint, tint};
		for (const Cell& c : cells)
		{
			if (!c.rect.Empty())
				AppendQuadVertices(c.rect, cs, c.uvMin, c.uvMax);
		}
	}

	void UIRenderer::Flush()
	{
		if (m_Vertices.empty() || m_Indices.empty() || !m_VAO)
		{
			m_Vertices.clear();
			m_Indices.clear();
			return;
		}

		const uint32_t program = (m_BatchShader == BatchShader::Text) ? m_TextProgram : m_Program;
		if (!program)
		{
			m_Vertices.clear();
			m_Indices.clear();
			return;
		}

		// Premul-alpha blending: src already premultiplied; standard
		// over-blend equation.
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		m_VBO->Bind();
		m_VBO->SetSubData(m_Vertices.data(), 0,
						  static_cast<uint32_t>(m_Vertices.size() * sizeof(UIVertex)));

		m_IBO->Bind();
		m_IBO->SetSubData(m_Indices.data(), 0,
						  static_cast<uint32_t>(m_Indices.size() * sizeof(uint32_t)));
		m_IBO->SetCount(static_cast<uint32_t>(m_Indices.size()));

		glUseProgram(program);
		if (m_BatchShader == BatchShader::Text)
		{
			glUniformMatrix4fv(m_TextLocProj, 1, GL_FALSE, glm::value_ptr(m_OrthoProjection));
			if (m_TextLocTex0 >= 0)
				glUniform1i(m_TextLocTex0, 0);
			if (m_TextLocPxRange >= 0)
				glUniform1f(m_TextLocPxRange, m_BatchPxRange);
		}
		else
		{
			glUniformMatrix4fv(m_LocProj, 1, GL_FALSE, glm::value_ptr(m_OrthoProjection));
			if (m_LocTex0 >= 0)
				glUniform1i(m_LocTex0, 0);
		}

		Onyx::Texture* tex = m_BatchTexture ? m_BatchTexture : m_WhitePixel.get();
		tex->Bind(0);

		m_VAO->Bind();
		glDrawElements(GL_TRIANGLES,
					   static_cast<GLsizei>(m_Indices.size()),
					   GL_UNSIGNED_INT, nullptr);

		++m_Stats.drawCalls;
		++m_Stats.batchBreaks;

		m_Vertices.clear();
		m_Indices.clear();
	}

	glm::vec2 UIRenderer::MeasureText(std::string_view text, const FontAtlas& font, float pxSize)
	{
		if (!font.IsLoaded() || text.empty())
			return glm::vec2(0.0f, font.GetMetrics().lineHeight * (pxSize / font.GetMetrics().bakeSize));

		const float scale = pxSize / font.GetMetrics().bakeSize;
		float pen = 0.0f;
		// Naive ASCII / Latin-1 / Latin-Extended-A iteration via byte-by-byte
		// UTF-8 decoding limited to the codepoint range we baked.
		size_t i = 0;
		while (i < text.size())
		{
			uint32_t cp = static_cast<uint8_t>(text[i]);
			size_t adv = 1;
			if ((cp & 0x80) == 0) { /* ASCII */ }
			else if ((cp & 0xE0) == 0xC0 && i + 1 < text.size())
			{
				cp = ((cp & 0x1F) << 6) | (static_cast<uint8_t>(text[i + 1]) & 0x3F);
				adv = 2;
			}
			else if ((cp & 0xF0) == 0xE0 && i + 2 < text.size())
			{
				cp = ((cp & 0x0F) << 12)
					 | ((static_cast<uint8_t>(text[i + 1]) & 0x3F) << 6)
					 | (static_cast<uint8_t>(text[i + 2]) & 0x3F);
				adv = 3;
			}
			else
			{
				adv = 1; // unknown byte; skip
			}
			i += adv;
			if (const Glyph* g = font.FindGlyph(cp))
				pen += g->advance * scale;
		}
		const float lineH = font.GetMetrics().lineHeight * scale;
		return glm::vec2(pen, lineH);
	}

	glm::vec2 UIRenderer::DrawText(const Rect2D& bounds, std::string_view text,
								   const FontAtlas& font, float pxSize,
								   const Color& color,
								   TextAlignH align)
	{
		if (!font.IsLoaded() || text.empty() || !font.GetTexture())
			return bounds.min;

		const FontMetrics& fm = font.GetMetrics();
		const float scale = pxSize / fm.bakeSize;
		const float screenPxRange = fm.pixelRange * scale;

		// Force a flush + switch to text shader. screenPxRange is a per-batch
		// uniform so different sizes need different batches; for the M3 first
		// cut we conservatively flush per DrawText call.
		Flush();
		m_BatchShader = BatchShader::Text;
		m_BatchTexture = font.GetTexture();
		m_BatchPxRange = screenPxRange;

		// Compute origin: bounds.min is the top-left of the text rect.
		// Pen starts at (origin.x, origin.y + ascender * scale) so glyphs sit
		// on the baseline.
		const glm::vec2 measured = MeasureText(text, font, pxSize);
		float originX = bounds.min.x;
		if (align == TextAlignH::Center)
			originX = bounds.min.x + (bounds.Width() - measured.x) * 0.5f;
		else if (align == TextAlignH::Right)
			originX = bounds.max.x - measured.x;

		const float baselineY = bounds.min.y + fm.ascender * scale;

		float pen = originX;

		size_t i = 0;
		while (i < text.size())
		{
			uint32_t cp = static_cast<uint8_t>(text[i]);
			size_t adv = 1;
			if ((cp & 0x80) == 0) {}
			else if ((cp & 0xE0) == 0xC0 && i + 1 < text.size())
			{
				cp = ((cp & 0x1F) << 6) | (static_cast<uint8_t>(text[i + 1]) & 0x3F);
				adv = 2;
			}
			else if ((cp & 0xF0) == 0xE0 && i + 2 < text.size())
			{
				cp = ((cp & 0x0F) << 12)
					 | ((static_cast<uint8_t>(text[i + 1]) & 0x3F) << 6)
					 | (static_cast<uint8_t>(text[i + 2]) & 0x3F);
				adv = 3;
			}
			i += adv;

			const Glyph* g = font.FindGlyph(cp);
			if (!g) continue;

			// Plane bounds are pixel offsets from baseline pen, in atlas-bake
			// pixel space. Scale to display size.
			if (g->uvMax.x > g->uvMin.x && g->uvMax.y > g->uvMin.y)
			{
				const Rect2D quad{
					glm::vec2(pen + g->planeMin.x * scale, baselineY + g->planeMin.y * scale),
					glm::vec2(pen + g->planeMax.x * scale, baselineY + g->planeMax.y * scale)
				};
				EnsureCapacity(1);
				const Color cs[4] = {color, color, color, color};
				AppendQuadVertices(quad, cs, g->uvMin, g->uvMax);
			}

			pen += g->advance * scale;
		}

		// Flush text batch immediately so the next draw can switch back to the
		// default shader without leaving stale state.
		Flush();
		m_BatchShader = BatchShader::Default;
		m_BatchTexture = nullptr;

		return glm::vec2(pen, baselineY);
	}

} // namespace Onyx::UI
