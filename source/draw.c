#include "log.h"
#include "maths.h"
#include "memory.h"
#include "render.h"
#include "strings.h"
#include "draw.h"

#include <assert.h>
#include <float.h>
#include <math.h>

static f32 LinearFromSRGBChannel(f32 C)
{
	return C <= 0.04045f ? C / 12.92f : powf((C + 0.055f) / 1.055f, 2.4f);
}

v4f LinearFromSRGB(v4f Colour)
{
	return v4f(LinearFromSRGBChannel(Colour.X),
	           LinearFromSRGBChannel(Colour.Y),
	           LinearFromSRGBChannel(Colour.Z),
	           Colour.W);
}

static u32 D_DecodeUTF8(str8 String, s64 *At)
{
	u8 *B = (u8*)String.Bytes;
	s64 I = *At;
	s64 Remaining = String.Size - I;
	u32 C = B[I];

	if (C < 0x80)
	{
		*At = I + 1;
		return C;
	}
	else if ((C & 0xE0) == 0xC0 && Remaining >= 2 &&
	         (B[I+1] & 0xC0) == 0x80)
	{
		*At = I + 2;
		return ((C & 0x1F) << 6) | (B[I+1] & 0x3F);
	}
	else if ((C & 0xF0) == 0xE0 && Remaining >= 3 &&
	         (B[I+1] & 0xC0) == 0x80 && (B[I+2] & 0xC0) == 0x80)
	{
		*At = I + 3;
		return ((C & 0x0F) << 12) | ((B[I+1] & 0x3F) << 6) | (B[I+2] & 0x3F);
	}
	else if ((C & 0xF8) == 0xF0 && Remaining >= 4 &&
	         (B[I+1] & 0xC0) == 0x80 && (B[I+2] & 0xC0) == 0x80 && (B[I+3] & 0xC0) == 0x80)
	{
		*At = I + 4;
		return ((C & 0x07) << 18) | ((B[I+1] & 0x3F) << 12) |
		       ((B[I+2] & 0x3F) << 6) | (B[I+3] & 0x3F);
	}

	*At = I + 1;
	return C;
}

d_font D_LoadFontFromTTF(draw_context *D, u8 *Data)
{
	d_font Font = {0};
	stbtt_fontinfo Info = {0};
	int Status = stbtt_InitFont(&Info, Data, 0);
	if (!Status)
	{
		LogError("Internal font file is invalid");
	}
	else
	{
		stbtt_GetFontVMetrics(&Info, &Font.Ascent, &Font.Descent, &Font.LineGap);
		Font.Info = Info;
	}
	return Font;
}

static void CurvesFromCodePoint(draw_context *Draw, m_arena *Arena,
                                d_atlas_glyph *Glyph, u32 CodePoint, d_font Font,
                                int *CurveCount, v2f **Curves)
{
	s32 GlyphIndex = stbtt_FindGlyphIndex(&Font.Info, CodePoint);
	stbtt_vertex *Vertices;
	s32 NumberOfVertices = stbtt_GetGlyphShape(&Font.Info, GlyphIndex, &Vertices);

	v4i Box = {0};
	stbtt_GetGlyphBox(&Font.Info, GlyphIndex, &Box.X, &Box.Y, &Box.Z, &Box.W);
	f32 Width = (f32)(Box.Z - Box.X);
	f32 Height = (f32)(Box.W - Box.Y);
	v2f Size = v2f(Width, Height);

	v2f Shift = v2f(Box.X, Box.Y);

	Curves[0] = ArenaPushArrayZero(Arena, v2f, NumberOfVertices * 3);
	CurveCount[0] = 0;

	v2f First = {0};
	for (s32 I = 0; I < NumberOfVertices; I++)
	{
		stbtt_vertex *V = Vertices + I;
		switch (V->type)
		{
		case STBTT_vmove:
			First = SubV2F(v2f(V->x, V->y), Shift);
			break;
		case STBTT_vcurve:
		{
			v2f Control = SubV2F(v2f(V->cx, V->cy), Shift);
			v2f End     = SubV2F(v2f(V->x,  V->y),  Shift);
			Curves[0][CurveCount[0]++] = End;
			Curves[0][CurveCount[0]++] = Control;
			Curves[0][CurveCount[0]++] = First;
			First = End;
		} break;
		case STBTT_vline:
		{
			v2f End = SubV2F(v2f(V->x, V->y), Shift);
			v2f Control = v2f((First.X + End.X)*0.5f, (First.Y + End.Y)*0.5f);
			Curves[0][CurveCount[0]++] = End;
			Curves[0][CurveCount[0]++] = Control;
			Curves[0][CurveCount[0]++] = First;
			First = End;
		} break;
		}
	}

	for (int I = 0; I < CurveCount[0]; I++)
	{
		Curves[0][I].X /= Size.X;
		Curves[0][I].Y = (Height - Curves[0][I].Y) / Size.Y;
	}

	stbtt_FreeShape(&Font.Info, Vertices);
	(void)Draw;
}

d_atlas_glyph *D_GlyphFromCodePoint(draw_context *D, d_font Font, u32 CodePoint)
{
	u64 Hash = M_FNVHashBytes(M_FNV_HASH_START, (u8*)&Font, sizeof(d_font));
	Hash = M_FNVHashBytes(Hash, (u8*)&CodePoint, sizeof(u32));

	d_atlas_glyph *Glyph = D->GlyphTable + (Hash % D_GLYPH_TABLE_SIZE);
	for (; Glyph->NextInHashSlot != Null && Glyph->Hash != Hash; Glyph = Glyph->NextInHashSlot);
	d_atlas_glyph *Result = Glyph;

	if (Glyph->Hash != 0 && Glyph->Hash != Hash)
	{
		d_atlas_glyph *Next = ArenaPushArrayZero(D->Arena, d_atlas_glyph, 1);
		Glyph->NextInHashSlot = Next;
		Glyph = Next;
		Result = Next;
	}

	if (Glyph->Hash == 0)
	{
		int AdvanceX;
		int LeftSideBearing;
		stbtt_GetCodepointHMetrics(&Font.Info, CodePoint, &AdvanceX, &LeftSideBearing);
		Glyph->AdvanceX = (f32)AdvanceX;
		Glyph->Hash = Hash;
		Glyph->CodePoint = CodePoint;
		stbtt_GetCodepointBox(&Font.Info, CodePoint, &Glyph->Box.X, &Glyph->Box.Y,
		                                              &Glyph->Box.Z, &Glyph->Box.W);

		CurvesFromCodePoint(D, D->Arena, Glyph, CodePoint, Font, &Glyph->CurveCount, &Glyph->Curves);
	}

	return Result;
}

// Defined further down; Quad/Point route through it so they pick up the same
// outline + extrusion + lighting as the rounded shapes.
static void PushSDFBox(draw_context *D, str8 Shader, v2f Position,
                       f32 Width, f32 Height, f32 Param, d_shape_options Options);

void D_BaseQuad(draw_context *D, v2f Position, f32 Width, f32 Height, d_shape_options Options)
{
	// Corner radius 0 → sharp rectangle, but flowing through the SDF path means it
	// inherits outline + extrusion + lighting like the rounded shapes.
	PushSDFBox(D, Str8("rounded_box.metal"), Position, Width, Height, 0.0f, Options);
}

f32 D_Str8WidthFromHeight(d_font Font, str8 String, f32 HeightInPixels)
{
	f32 Width = 0.0f;
	f32 Scale = stbtt_ScaleForPixelHeight(&Font.Info, HeightInPixels);
	for (s64 I = 0; I < String.Size; )
	{
		int CP = (int)D_DecodeUTF8(String, &I);
		s64 NextAt = I;
		int NextCP = (NextAt < String.Size) ? (int)D_DecodeUTF8(String, &NextAt) : 0;
		int GlyphIndex = stbtt_FindGlyphIndex(&Font.Info, CP);
		int Advance, LSB;
		stbtt_GetGlyphHMetrics(&Font.Info, GlyphIndex, &Advance, &LSB);
		int NextGlyphIndex = stbtt_FindGlyphIndex(&Font.Info, NextCP);
		int Kern = stbtt_GetGlyphKernAdvance(&Font.Info, GlyphIndex, NextGlyphIndex);
		Width += (Kern + Advance) * Scale;
	}
	return Width;
}

f32 D_LineHeightFromFont(d_font Font, f32 HeightInPixels)
{
	f32 Scale = stbtt_ScaleForPixelHeight(&Font.Info, HeightInPixels);
	return (Font.Ascent - Font.Descent + Font.LineGap) * Scale;
}

f32 D_Str8HeightFromHeight(d_font Font, f32 HeightInPixels)
{
	f32 Scale = stbtt_ScaleForPixelHeight(&Font.Info, HeightInPixels);
	return (f32)(Font.Ascent - Font.Descent) * Scale;
}

static f32 D_AlignFraction(d_align Align)
{
	switch (Align)
	{
		case D_ALIGN_CENTRE: return 0.5f;
		case D_ALIGN_MAX:    return 1.0f;
		case D_ALIGN_MIN:    return 0.0f;
		default:             return 0.0f;
	}
}

v2f D_Str8AlignInBox(d_font Font, str8 String, f32 HeightInPixels,
                     v2f BoxPosition, v2f BoxSize, d_align AlignX, d_align AlignY)
{
	f32 Scale = stbtt_ScaleForPixelHeight(&Font.Info, HeightInPixels);
	f32 TextW = D_Str8WidthFromHeight(Font, String, HeightInPixels);
	f32 TextH = (f32)(Font.Ascent - Font.Descent) * Scale;

	f32 PenX = BoxPosition.X + (BoxSize.X - TextW) * D_AlignFraction(AlignX);

	f32 BoxBottomY = BoxPosition.Y + (BoxSize.Y - TextH) * D_AlignFraction(AlignY);
	f32 PenY = BoxBottomY - (f32)Font.Descent * Scale;

	return v2f(PenX, PenY);
}

void D_BaseStr8InBox(draw_context *D, str8 String,
                     v2f BoxPosition, v2f BoxSize, d_align AlignX, d_align AlignY,
                     d_str8_options Options)
{
	v2f Pen = D_Str8AlignInBox(Options.Font, String, Options.HeightInPixels,
	                           BoxPosition, BoxSize, AlignX, AlignY);
	Options.Position.X = Pen.X;
	Options.Position.Y = Pen.Y;
	D_BaseStr8(D, String, Options);
}

// Copy a glyph's curves into this frame's curve buffer (allocated lazily on the
// first glyph of the frame), returning the offset. Deduplicated per frame, so a
// repeated letter is packed once; glyphs not drawn this frame cost nothing.
static int D_PackGlyphCurves(draw_context *D, d_atlas_glyph *Glyph)
{
	render_context *R = D->Render;
	u64 Stamp = R->Frame + 1;

	if (D->CurveFrameStamp != Stamp)
	{
		D->CurveBuffer = R_BufferFrame(R, Str8("Glyph Curves"), .SizeInBytes = MegaBytes(4));
		D->CurveData = (v2f*)R_BufferData(R, D->CurveBuffer);
		D->CurveCursor = 0;
		D->CurveFrameStamp = Stamp;
	}

	if (Glyph->FramePackStamp != Stamp)
	{
		MemoryCopy(D->CurveData + D->CurveCursor, Glyph->Curves, Glyph->CurveCount * sizeof(v2f));
		Glyph->FrameCurveOffset = D->CurveCursor;
		Glyph->FramePackStamp = Stamp;
		D->CurveCursor += Glyph->CurveCount;
	}
	return Glyph->FrameCurveOffset;
}

void D_BaseStr8(draw_context *D, str8 String, d_str8_options Options)
{
	render_context *R = D->Render;
	f32 StartX = Options.Position.X;
	v4f Colour = LinearFromSRGB(Options.Colour);

	assert(Options.HeightInPixels > 0.0f);
	f32 Scale = stbtt_ScaleForPixelHeight(&Options.Font.Info, Options.HeightInPixels);

	for (s64 I = 0; I < String.Size; )
	{
		u32 CodePoint = D_DecodeUTF8(String, &I);
		s64 NextAt = I;
		u32 NextCodePoint = (NextAt < String.Size) ? D_DecodeUTF8(String, &NextAt) : 0;

		if (CodePoint == '\r') continue;
		f32 TabMultiplier = 1.0f;
		if (CodePoint == '\t') { CodePoint = ' '; TabMultiplier = 4.0f; }

		if (CodePoint == '\n')
		{
			Options.Position.Y -= (Options.Font.Ascent - Options.Font.Descent + Options.Font.LineGap) * Scale;
			Options.Position.X = StartX;
			continue;
		}

		d_atlas_glyph *Glyph = D_GlyphFromCodePoint(D, Options.Font, CodePoint);
		int Kern = stbtt_GetCodepointKernAdvance(&Options.Font.Info, CodePoint, NextCodePoint);
		f32 AdvanceX = (Kern + Glyph->AdvanceX) * Scale * TabMultiplier;

		f32 GlyphWidth  = ((f32)(Glyph->Box.Z - Glyph->Box.X) * Scale);
		f32 GlyphHeight = ((f32)(Glyph->Box.W - Glyph->Box.Y) * Scale);

		if (CodePoint != ' ' && Glyph->CurveCount > 0)
		{
			int CurveOffset = D_PackGlyphCurves(D, Glyph);
			f32 X0 = Options.Position.X + Glyph->Box.X * Scale;
			f32 Y0 = Options.Position.Y + Glyph->Box.Y * Scale;
			R_UniformM44F(R, Str8("PVMMatrix"),  R_CurrentTransform(R));
			R_UniformV2F (R, Str8("QuadMin"),    v2f(X0, Y0));
			R_UniformV2F (R, Str8("QuadMax"),    v2f(X0 + GlyphWidth, Y0 + GlyphHeight));
			R_UniformV4F (R, Str8("Colour"),     Colour);
			R_UniformU32 (R, Str8("CurveStart"), (u32)CurveOffset);
			R_UniformU32 (R, Str8("CurveCount"), (u32)Glyph->CurveCount);
			R_BindBuffer (R, D->CurveBuffer);
			R_DrawTriangles(R, Str8("glyph.metal"), 6);
		}

		Options.Position.X += AdvanceX;
	}
}

void D_Line(draw_context *D, v2f Start, v2f End, f32 Thickness, v4f Colour)
{
	render_context *R = D->Render;
	f32 Pad = Thickness * 0.5f + 1.0f;
	v2f Min = v2f(fminf(Start.X, End.X) - Pad, fminf(Start.Y, End.Y) - Pad);
	v2f Max = v2f(fmaxf(Start.X, End.X) + Pad, fmaxf(Start.Y, End.Y) + Pad);

	R_UniformM44F(R, Str8("PVMMatrix"), R_CurrentTransform(R));
	R_UniformV2F (R, Str8("QuadMin"),   Min);
	R_UniformV2F (R, Str8("QuadMax"),   Max);
	R_UniformV4F (R, Str8("Colour"),    LinearFromSRGB(Colour));
	R_UniformV2F (R, Str8("Start"),     Start);
	R_UniformV2F (R, Str8("End"),       End);
	R_UniformF32 (R, Str8("Thickness"), Thickness);
	R_DrawTriangles(R, Str8("line.metal"), 6);
}

void D_Bezier(draw_context *D, v2f Start, v2f Control, v2f End, f32 Thickness, v4f Colour)
{
	render_context *R = D->Render;
	f32 Pad = Thickness * 0.5f + 1.0f;
	f32 MinX = fminf(Start.X, fminf(Control.X, End.X)) - Pad;
	f32 MinY = fminf(Start.Y, fminf(Control.Y, End.Y)) - Pad;
	f32 MaxX = fmaxf(Start.X, fmaxf(Control.X, End.X)) + Pad;
	f32 MaxY = fmaxf(Start.Y, fmaxf(Control.Y, End.Y)) + Pad;

	R_UniformM44F(R, Str8("PVMMatrix"), R_CurrentTransform(R));
	R_UniformV2F (R, Str8("QuadMin"),   v2f(MinX, MinY));
	R_UniformV2F (R, Str8("QuadMax"),   v2f(MaxX, MaxY));
	R_UniformV4F (R, Str8("Colour"),    LinearFromSRGB(Colour));
	R_UniformV2F (R, Str8("Start"),     Start);
	R_UniformV2F (R, Str8("Control"),   Control);
	R_UniformV2F (R, Str8("End"),       End);
	R_UniformF32 (R, Str8("Thickness"), Thickness);
	R_DrawTriangles(R, Str8("bezier.metal"), 6);
}

static void EmitSDFQuad(draw_context *D, str8 Shader, v2f Min, v2f Max,
                        v4f Colour, v2f Centre, v2f HalfSize, f32 Param,
                        v4f OutlineColour, f32 OutlineThickness, f32 Extrusion,
                        v4f CornerRadii, f32 ShadowBlur)
{
	render_context *R = D->Render;
	R_UniformM44F(R, Str8("PVMMatrix"),        R_CurrentTransform(R));
	R_UniformV2F (R, Str8("QuadMin"),          Min);
	R_UniformV2F (R, Str8("QuadMax"),          Max);
	R_UniformV4F (R, Str8("Colour"),           Colour);
	R_UniformV2F (R, Str8("Centre"),           Centre);
	R_UniformV2F (R, Str8("HalfSize"),         HalfSize);
	R_UniformF32 (R, Str8("Param"),            Param);
	R_UniformV4F (R, Str8("OutlineColour"),    OutlineColour);
	R_UniformF32 (R, Str8("OutlineThickness"), OutlineThickness);
	R_UniformF32 (R, Str8("Extrusion"),        Extrusion);
	R_UniformV4F (R, Str8("CornerRadii"),      CornerRadii);
	R_UniformF32 (R, Str8("ShadowBlur"),       ShadowBlur);
	R_UniformV4F (R, Str8("LightPosition"),    v4f(R->LightPosition.X, R->LightPosition.Y, R->LightPosition.Z, R->LightRadius));
	R_UniformV4F (R, Str8("LightColour"),      v4f(R->LightColour.X, R->LightColour.Y, R->LightColour.Z, 0));
	R_UniformF32 (R, Str8("LightIntensity"),   R->LightIntensity);
	R_DrawTriangles(R, Shader, 6);
}

static void PushSDFBox(draw_context *D, str8 Shader, v2f Position,
                       f32 Width, f32 Height, f32 Param, d_shape_options Options)
{
	v2f HalfSize = v2f(Width * 0.5f, Height * 0.5f);
	v2f Centre   = v2f(Position.X + HalfSize.X, Position.Y + HalfSize.Y);

	// Per-corner radii default to the uniform scalar (Param) when the caller
	// leaves Options.CornerRadii zeroed, so single-radius shapes are unchanged.
	v4f CornerRadii = Options.CornerRadii;
	if (CornerRadii.X == 0.0f && CornerRadii.Y == 0.0f &&
	    CornerRadii.Z == 0.0f && CornerRadii.W == 0.0f)
	{
		CornerRadii = v4f(Param, Param, Param, Param);
	}

	if (Options.ShadowBlur > 0.0f)
	{
		v4f ShadowColour = Options.ShadowColour;
		if (ShadowColour.X == 0.0f && ShadowColour.Y == 0.0f &&
		    ShadowColour.Z == 0.0f && ShadowColour.W == 0.0f)
		{
			ShadowColour = v4f(0.0f, 0.0f, 0.0f, 0.35f);
		}

		v2f SPos = AddV2F(Position, Options.ShadowOffset);
		f32 SPad = 1.0f + Options.ShadowBlur;
		v2f SMin = v2f(SPos.X - SPad,         SPos.Y - SPad);
		v2f SMax = v2f(SPos.X + Width + SPad, SPos.Y + Height + SPad);

		EmitSDFQuad(D, Shader, SMin, SMax,
			LinearFromSRGB(ShadowColour),
			v2f(SPos.X + HalfSize.X, SPos.Y + HalfSize.Y),
			HalfSize, Param,
			v4f(0, 0, 0, 0), 0.0f, 0.0f,
			CornerRadii, Options.ShadowBlur);
	}

	// Pad enough for AA on the edge plus the outline (drawn just inside, but
	// AA can spill 1px outside).
	f32 Pad = 1.0f + fmaxf(0.0f, Options.OutlineThickness);
	v2f Min = v2f(Position.X - Pad,         Position.Y - Pad);
	v2f Max = v2f(Position.X + Width + Pad, Position.Y + Height + Pad);

	EmitSDFQuad(D, Shader, Min, Max,
		LinearFromSRGB(Options.Colour),
		Centre, HalfSize, Param,
		LinearFromSRGB(Options.OutlineColour), Options.OutlineThickness, Options.Extrusion,
		CornerRadii, 0.0f);
}

void D_BaseRoundedQuad(draw_context *D, v2f Position, f32 Width, f32 Height, f32 CornerRadius, d_shape_options Options)
{
	PushSDFBox(D, Str8("rounded_box.metal"), Position, Width, Height, CornerRadius, Options);
}

void D_BaseCapsule(draw_context *D, v2f Position, f32 Width, f32 Height, d_shape_options Options)
{
	f32 Radius = 0.5f * fminf(Width, Height);
	PushSDFBox(D, Str8("rounded_box.metal"), Position, Width, Height, Radius, Options);
}

void D_BaseSquircle(draw_context *D, v2f Position, f32 Width, f32 Height, f32 N, d_shape_options Options)
{
	PushSDFBox(D, Str8("squircle.metal"), Position, Width, Height, N, Options);
}

void D_BasePoint(draw_context *D, v2f Position, f32 Size, d_shape_options Options)
{
	// A rounded box whose corner radius equals its half-size is a circle.
	// PushSDFBox expects the top-left corner; D_Point's Position is the centre.
	f32 Half = Size * 0.5f;
	v2f Corner = v2f(Position.X - Half, Position.Y - Half);
	PushSDFBox(D, Str8("rounded_box.metal"), Corner, Size, Size, Half, Options);
}
