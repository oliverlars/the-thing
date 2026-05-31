#include "misc.h"
#include "memory.h"
#include "strings.h"
#include "log.h"
#include "maths.h"
#include "os.h"
#include "render.h"
#include "draw.h"

#include <math.h>

typedef struct app_state
{
	m_arena *FrameArenas[2];
	int FrameArenaIndex;
	render_context Render;
	draw_context Draw;
	os_window *Window;
	d_font Font;
	u64 StartTime;
} app_state;

static void Frame(void *UserData, v2i ViewportSize)
{
	app_state *App = (app_state*)UserData;

	App->FrameArenaIndex = 1 - App->FrameArenaIndex;
	m_arena *Frame = App->FrameArenas[App->FrameArenaIndex];
	ArenaReset(Frame);
	SetTemporaryArena(Frame);

	App->Draw.ViewportSize = v2f(ViewportSize.X, ViewportSize.Y);
	App->Render.CommandsFirst = NULL;
	App->Render.CommandsLast = NULL;
	App->Render.TransformCount = 0;
	App->Render.ScissorCount = 0;

	m44f Ortho = M44FOrthographic(0, ViewportSize.X, ViewportSize.Y, 0, -1000, 1000);
	R_PushTransform(&App->Render, Ortho);
	R_PushScissor(&App->Render, v4i(0, 0, ViewportSize.X, ViewportSize.Y));
	R_BeginFrame(&App->Render, ViewportSize);

	f32 W = (f32)ViewportSize.X;
	f32 H = (f32)ViewportSize.Y;

	// Light follows the mouse cursor; .z hovers it above the screen plane.
	v2f Mouse = OS_MousePosition(App->Window);
	v3f LightPos = v3f(Mouse.X, Mouse.Y, 200.0f);
	R_SetLight(&App->Render, LightPos, v3f(1.0f, 1.0f, 1.0f), 400.0f);

	f32 BX0 = W * 0.20f;
	f32 BX1 = W * 0.80f;
	f32 BMid = (BX0 + BX1) * 0.5f;
	f32 BY  = H * 0.50f;
	f32 BCY = H * 0.70f;

	D_Quad(&App->Draw, v2f(BX0 - 4, BY - 4), (BX1 - BX0) + 8, (BCY - BY) + 8,
		.Colour = v4f(0.25f, 0.25f, 0.25f, 1.0f));

	D_Bezier(&App->Draw,
		v2f(BX0,  BY),
		v2f(BMid, BCY),
		v2f(BX1,  BY),
		6.0f,
		v4f(1.0f, 0.3f, 0.85f, 1.0f));

	int Segments = 32;
	v2f Prev = v2f(BX0, BY);
	for (int I = 1; I <= Segments; I++)
	{
		f32 T = (f32)I / (f32)Segments;
		f32 OneMinusT = 1.0f - T;
		f32 X = OneMinusT*OneMinusT*BX0 + 2*OneMinusT*T*BMid + T*T*BX1;
		f32 Y = OneMinusT*OneMinusT*BY  + 2*OneMinusT*T*BCY  + T*T*BY;
		v2f P = v2f(X, Y);
		D_Line(&App->Draw, Prev, P, 2.0f, v4f(0.3f, 0.95f, 0.4f, 1.0f));
		Prev = P;
	}

	D_Point(&App->Draw, v2f(BX0, BY), 20.0f, .Colour = v4f(1, 0.9f, 0.2f, 1));
	D_Point(&App->Draw, v2f(BX1, BY), 20.0f, .Colour = v4f(0.2f, 0.9f, 1.0f, 1));

	f32 ShapeY = H * 0.12f;
	f32 ShapeSize = 96.0f;
	f32 ShapeGap = 32.0f;

	// Flat rounded quad.
	D_RoundedQuad(&App->Draw, v2f(W * 0.10f, ShapeY),
		ShapeSize, ShapeSize, 20.0f,
		.Colour = v4f(0.95f, 0.45f, 0.30f, 1));

	// Capsule with an outline, also flat.
	D_Capsule(&App->Draw, v2f(W * 0.10f + (ShapeSize + ShapeGap), ShapeY),
		ShapeSize * 2.0f, ShapeSize * 0.55f,
		.Colour = v4f(0.35f, 0.85f, 0.55f, 1),
		.OutlineColour = v4f(1, 1, 1, 1),
		.OutlineThickness = 3.0f);

	// Squircles with varying exponent. The middle one opts into a soft sheen
	// that tracks the cursor; the others stay flat for comparison.
	f32 SquircleStartX = W * 0.10f + (ShapeSize + ShapeGap) + ShapeSize * 2.0f + ShapeGap;
	f32 Exponents[] = { 2.5f, 4.0f, 8.0f };
	for (int I = 0; I < (int)(sizeof(Exponents)/sizeof(Exponents[0])); I++)
	{
		f32 X = SquircleStartX + (ShapeSize + ShapeGap) * (f32)I;
		D_Squircle(&App->Draw, v2f(X, ShapeY),
			ShapeSize, ShapeSize, Exponents[I],
			.Colour = v4f(0.55f, 0.65f, 1.0f, 1),
			.OutlineColour = v4f(0.1f, 0.1f, 0.15f, 1),
			.OutlineThickness = 2.0f);
	}

	// Oscillates Extrusion between −1 (recessed) and +1 (raised) on a 2s
	// period to make the raise↔recess transition visible at a glance.
	f32 ElapsedSec = OS_MilliSecondsFromTimeStamp(OS_TimeStampNow() - App->StartTime) * 0.001f;
	v4f CapsuleColours[2] = { v4f(1.0f, 0.4f, 0.0f, 1), v4f(0.8f, 0.35f, 0.0f, 1) };
	static b32 Clicked = False;
	static f32 ExtrusionTarget = 1.0;
	static f32 Extrusion = 1.0;

	Extrusion = LerpF32(Extrusion, ExtrusionTarget, 0.1);
	v2f MousePosition = OS_MousePosition(App->Window);
	if (MousePosition.X >= 500 && MousePosition.X < 750 &&
		MousePosition.Y >= 500 && MousePosition.Y <= 580 && OS_MousePressed(OS_MOUSE_LEFT)) {
			ExtrusionTarget = (f32)Clicked;
			Clicked = !Clicked;
		}
	D_Capsule(&App->Draw, v2f(500, 500),
		250, 80,
		.Colour = CapsuleColours[Clicked],
		.OutlineColour = v4f(1.0f, 0.5f, 0.0f, 1),
		.OutlineThickness = 4.0f,
		.Extrusion = Extrusion);
	str8 Label = Clicked ? Str8("ON!") : Str8("OFF!");
	v4f LabelColour = Clicked ? v4f(0.45f, 0.05f, 0.0f, 1) : v4f(1.0f, 0.80f, 0.20f, 1);
	D_Str8InBox(&App->Draw, Label,
		v2f(500, 500), v2f(250, 80),
		D_ALIGN_CENTRE, D_ALIGN_CENTRE,
		.Font = App->Font,
		.HeightInPixels = 48.0f,
		.Colour = LabelColour);
	D_Str8(&App->Draw, Str8("HELLO, WORLD!"),
		.Font = App->Font,
		.Position = v3f(W * 0.10f, H * 0.90f, 0),
		.HeightInPixels = 64.0f,
		.Colour = v4f(1, 1, 1, 1));

	// DEBUG: render a magenta dot at whatever OS_MousePosition reports. If
	// this dot sits under the OS cursor the mouse coords are correct; if
	// it's offset (e.g. flipped vertically) the offset tells us the bug.
	D_Point(&App->Draw, Mouse, 16.0f, .Colour = v4f(1.0f, 0.0f, 1.0f, 1));

	R_Update(&App->Render, OS_NextDrawable(App->Window));
}

int main(int ArgCount, char **Args)
{
	(void)ArgCount; (void)Args;

	m_arena *Arena = ArenaInitialise(GigaBytes(4));
	per_thread_context *Ctx = GetThreadLocalContext();
	Ctx->ScratchArenas[0] = ArenaInitialise(GigaBytes(1));
	Ctx->ScratchArenas[1] = ArenaInitialise(GigaBytes(1));
	Ctx->LogArena = ArenaInitialise(MegaBytes(64));
	Ctx->Logger = LoggerInitialise(Ctx->LogArena);
	Ctx->Logger->Arena = Ctx->LogArena;

	app_state App = {0};
	App.FrameArenas[0] = ArenaInitialise(MegaBytes(256));
	App.FrameArenas[1] = ArenaInitialise(MegaBytes(256));
	SetTemporaryArena(App.FrameArenas[0]);

	os_window Window = OS_MakeWindow(Str8("the-thing"), 1280, 800);
	App.Window = &Window;

	r_init_options Opts = {
		.Device = Window.Device,
		.Layer = Window.Layer,
		.ShaderDir = Str8("shaders"),
	};
	R_Init(&App.Render, Arena, v2i(Window.Width, Window.Height), Opts);

	App.Draw.Arena = Arena;
	App.Draw.Render = &App.Render;

	str8 FontBytes = OS_LoadEntireFileIntoMemory(Arena, Str8("fonts/BerkeleyMono-Regular.ttf"));
	App.Font = D_LoadFontFromTTF(&App.Draw, FontBytes.Bytes);

	App.StartTime = OS_TimeStampNow();

	OS_Run(&Window, Frame, &App);
	return 0;
}
