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

void D_Line(draw_context *D, v2f Start, v2f End, f32 Thickness, v4f Colour);

void D_Bezier(draw_context *D, v2f Start, v2f Control, v2f End, f32 Thickness, v4f Colour);

void D_Point(draw_context *D, v2f Position, f32 Size, v4f Colour);

f32 D_LineHeightFromFont(d_font Font, f32 HeightInPixels);
