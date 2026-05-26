#pragma once
#include "memory.h"
#include "render.h"
#include "stb_truetype.h"

typedef struct d_font
{
	stbtt_fontinfo Info;
	int Ascent;
	int Descent;
	int LineGap;
} d_font;

typedef struct d_atlas_glyph
{
	u64 Hash;
	v4i Box;
	u32 CodePoint;
	f32 AdvanceX;

	v2f *Curves;
	int CurveStart;
	int CurveCount;

	struct d_atlas_glyph *NextInHashSlot;
} d_atlas_glyph;

#define D_GLYPH_TABLE_SIZE (8192)

typedef struct draw_context
{
	m_arena *Arena;
	render_context *Render;

	v2f ViewportSize;

	d_atlas_glyph GlyphTable[D_GLYPH_TABLE_SIZE];

	int CurveCount;
} draw_context;

d_atlas_glyph *D_GlyphFromCodePoint(draw_context *D_Context,
                                    d_font Font,
                                    u32 CodePoint);

d_font D_LoadFontFromFile(draw_context *D_Context, str8 FileName);
d_font D_LoadFontFromTTF(draw_context *D_Context, u8 *Data);

typedef struct d_str8_options
{
	d_font Font;
	v3f Position;
	f32 HeightInPixels;
	v4f Colour;
}
d_str8_options;

#define D_Str8(Draw, String, ...) \
	D_BaseStr8(Draw, String, (d_str8_options) { __VA_ARGS__ } )

f32 D_Str8WidthFromHeight(d_font Font, str8 String, f32 HeightInPixels);

void D_BaseStr8(draw_context *D_Context, str8 String, d_str8_options Options);

void D_Quad(draw_context *D, v2f Position, f32 Width, f32 Height, v4f Colour);

// Options shared by all 2D SDF shapes (rounded quad, capsule, squircle).
// Designed for designated-initializer use via the variadic macros below — any
// field left zero takes a sensible default.
typedef struct d_shape_options
{
	v4f Colour;

	// Outline rendered just inside the shape's edge. Thickness in pixels;
	// Thickness <= 0 disables the outline.
	v4f OutlineColour;
	f32 OutlineThickness;

	// Multiplier on the bevel normal's z component. Controls how the rim
	// reads under the point light:
	//   +1 (default): raised — bevel sits above the surface plane.
	//    0          : flat normal — purely lateral rim, no z-axis influence.
	//   -1         : recessed — bevel reads as pressed into the surface.
	// Animate this for a smooth raise/recess transition. If left zero in the
	// shape options, the constructor substitutes +1 (default-raised). Pass an
	// explicit non-zero value (e.g. 1e-3 ≈ 0) to opt out of that substitution.
	f32 Extrusion;
}
d_shape_options;

void D_BaseRoundedQuad(draw_context *D, v2f Position, f32 Width, f32 Height, f32 CornerRadius, d_shape_options Options);
void D_BaseCapsule    (draw_context *D, v2f Position, f32 Width, f32 Height,                   d_shape_options Options);
void D_BaseSquircle   (draw_context *D, v2f Position, f32 Width, f32 Height, f32 N,            d_shape_options Options);

#define D_RoundedQuad(D, Pos, W, H, Radius, ...) \
	D_BaseRoundedQuad(D, Pos, W, H, Radius, (d_shape_options){ __VA_ARGS__ })

// A capsule is just a rounded rectangle whose corner radius is half the short
// side, so this is a convenience wrapper around D_RoundedQuad.
#define D_Capsule(D, Pos, W, H, ...) \
	D_BaseCapsule(D, Pos, W, H, (d_shape_options){ __VA_ARGS__ })

// Apple-style squircle (superellipse). Exponent N: 2 = ellipse, 4 = classic
// squircle, ~5 matches the iOS app-icon shape, higher → more rectangular.
#define D_Squircle(D, Pos, W, H, N, ...) \
	D_BaseSquircle(D, Pos, W, H, N, (d_shape_options){ __VA_ARGS__ })

void D_Line(draw_context *D, v2f Start, v2f End, f32 Thickness, v4f Colour);

void D_Bezier(draw_context *D, v2f Start, v2f Control, v2f End, f32 Thickness, v4f Colour);

void D_Point(draw_context *D, v2f Position, f32 Size, v4f Colour);

f32 D_LineHeightFromFont(d_font Font, f32 HeightInPixels);
