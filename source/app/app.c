#include <math.h>

#include "misc.h"
#include "memory.h"
#include "render_common.h"
#include "strings.h"
#include "log.h"
#include "maths.h"
#include "os.h"
#include "render.h"
#include "draw.h"
#include "ui.h"
#include "embeds.h"


#include "new.h"

#include "app.h"


typedef struct app
{
	os_window   Window;
	per_thread_context *ThreadContext;
	m_arena *PermanentArena;
	m_arena *FrameArenas[2];
	int FrameArenaIndex;
	render_context Render;
	draw_context Draw;
	d_font Font;
	d_font LucideFont;
	u64 StartTime;

	ui_context UI;
} app;




// Perspective projection matching Metal's clip-space depth range [0, 1].
// The shared M44FPerspective is OpenGL-style ([-1, 1]), which would clip the
// near half of the cube against Metal's z >= 0 plane. Row-vector convention
// (point is transformed as v * M), camera looking down -Z.
static m44f PerspectiveMetal(f32 Aspect, f32 FOVDegrees, f32 Near, f32 Far)
{
	f32 FOV = (FOVDegrees / 360.0f) * 2.0f * (f32)M_PI;
	f32 F = 1.0f / tanf(FOV / 2.0f);
	m44f R = {0};
	R.E[0][0] = F / Aspect;
	R.E[1][1] = F;
	R.E[2][2] = -Far / (Far - Near);
	R.E[2][3] = -1.0f;
	R.E[3][2] = -(Far * Near) / (Far - Near);
	return R;
}


void Init(app *App, m_arena *PermanentArena)
{
	App->Window = OS_MakeWindow(Str8("the thing"), 1280, 720);

	r_init_options Opts = {
		.Device = App->Window.Device,
		.Layer = App->Window.Layer,
		.ShaderDir = Str8("shaders"),
		.Logging = False,
	};

	R_Init(&App->Render, PermanentArena, v2i(App->Window.Width, App->Window.Height), Opts);

	App->Draw.Arena = PermanentArena;
	App->Draw.Render = &App->Render;

	App->Font = D_LoadFontFromTTF(&App->Draw, E_BerkeleyMono);
	App->LucideFont = D_LoadFontFromTTF(&App->Draw, E_Lucide);

	App->StartTime = OS_TimeStampNow();

	App->UI.Draw = &App->Draw;
	App->UI.Font = App->Font;
	App->UI.IconFont = App->LucideFont;
	App->UI.Window = &App->Window;
}

app *ClientMemory()
{
	m_arena *PermanentArena = ArenaInitialise(GigaBytes(8));
	app *App = ArenaPushArrayZero(PermanentArena, app, 1);
	App->PermanentArena = PermanentArena;
	App->FrameArenas[0] = ArenaInitialise(GigaBytes(8));
	App->FrameArenas[1] = ArenaInitialise(GigaBytes(8));
	
	per_thread_context *ThreadContext = ArenaPushArrayZero(App->PermanentArena, per_thread_context, 1);

	ThreadContext->ScratchArenas[0] = ArenaInitialise(MegaBytes(64));
	ThreadContext->ScratchArenas[1] = ArenaInitialise(MegaBytes(64));
	ThreadContext->TemporaryArena = ArenaInitialise(GigaBytes(4));
	ThreadContext->LogArena = ArenaInitialise(MegaBytes(8));
	ThreadContext->Logger = LoggerInitialise(ThreadContext->LogArena);

	App->ThreadContext = ThreadContext;
	SetThreadLocalContextPointer(ThreadContext);

	Init(App, App->PermanentArena);

	return App;
}

void AppUpdate(void *UserData)
{
	app_interface *Interface = (app_interface*)UserData;
	if(!Interface->App) Interface->App = ClientMemory();

	app *App = Interface->App;

	SetThreadLocalContextPointer(App->ThreadContext);


	Interface->WantsToQuit = App->Window.WantsToQuit;
	if(!OS_UpdateWindow(&App->Window)) return;

	OS_BeginFrame();

	App->FrameArenaIndex = 1 - App->FrameArenaIndex;
	m_arena *Frame = App->FrameArenas[App->FrameArenaIndex];
	ArenaReset(Frame);
	SetTemporaryArena(Frame);

	v2i ViewportSize = v2i(App->Window.Width, App->Window.Height);

	App->Render.TransformCount = 0;
	App->Render.ScissorCount = 0;



	m44f Ortho = M44FOrthographic(0, ViewportSize.X, ViewportSize.Y, 0, -1000, 1000);
	R_PushTransform(&App->Render, Ortho);
	R_PushScissor(&App->Render, v4i(0, 0, ViewportSize.X, ViewportSize.Y));
	R_BeginFrame(&App->Render, ViewportSize);

	f32 W = (f32)ViewportSize.X;
	f32 H = (f32)ViewportSize.Y;

	// Light follows the mouse cursor; .z hovers it above the screen plane.
	v2f Mouse = OS_MousePosition(&App->Window);
	v3f LightPos = v3f(Mouse.X, Mouse.Y, 200.0f);
	R_SetLight(&App->Render, LightPos, v3f(1.0f, 1.0f, 1.0f), 1.0, 400.0f);


	f32 BX0 = W * 0.20f;
	f32 BX1 = W * 0.80f;
	f32 BMid = (BX0 + BX1) * 0.5f;
	f32 BY  = H * 0.50f;
	f32 BCY = H * 0.70f;

	// D_Quad(&App->Draw, v2f(BX0 - 4, BY - 4), (BX1 - BX0) + 8, (BCY - BY) + 8,
	// 	.Colour = v4f(0.25f, 0.25f, 0.25f, 1.0f));

	// D_Bezier(&App->Draw,
	// 	v2f(BX0,  BY),
	// 	v2f(BMid, BCY),
	// 	v2f(BX1,  BY),
	// 	6.0f,
	// 	v4f(1.0f, 0.3f, 0.85f, 1.0f));

	// int Segments = 32;
	// v2f Prev = v2f(BX0, BY);
	// for (int I = 1; I <= Segments; I++)
	// {
	// 	f32 T = (f32)I / (f32)Segments;
	// 	f32 OneMinusT = 1.0f - T;
	// 	f32 X = OneMinusT*OneMinusT*BX0 + 2*OneMinusT*T*BMid + T*T*BX1;
	// 	f32 Y = OneMinusT*OneMinusT*BY  + 2*OneMinusT*T*BCY  + T*T*BY;
	// 	v2f P = v2f(X, Y);
	// 	D_Line(&App->Draw, Prev, P, 2.0f, v4f(0.3f, 0.95f, 0.4f, 1.0f));
	// 	Prev = P;
	// }

	// D_Point(&App->Draw, v2f(BX0, BY), 20.0f, .Colour = v4f(1, 0.9f, 0.2f, 1));
	// D_Point(&App->Draw, v2f(BX1, BY), 20.0f, .Colour = v4f(0.2f, 0.9f, 1.0f, 1));



	// R_Pass(&App->Render, Str8("Pass test"), .Colours[0] = TestTexture, .ColourOps[0] = LO_CLEAR, .ClearColour[0] = v4f(1, 0, 0, 0))
	// {
	// 	R_BindBuffer(&App->Render, Vertices);
	// 	R_UniformF32(&App->Render, Str8("SomeUniform"), 0.0);
	// 	R_DrawTriangles(&App->Render, Str8("triangle2.metal"), 3);
	// }

	v3f Corners[8] = {
		v3f(-0.5f, -0.5f, -0.5f), v3f( 0.5f, -0.5f, -0.5f),
		v3f( 0.5f,  0.5f, -0.5f), v3f(-0.5f,  0.5f, -0.5f),
		v3f(-0.5f, -0.5f,  0.5f), v3f( 0.5f, -0.5f,  0.5f),
		v3f( 0.5f,  0.5f,  0.5f), v3f(-0.5f,  0.5f,  0.5f),
	};

	struct { int I[4]; v4f Colour; } Faces[6] = {
		{ { 4, 5, 6, 7 }, v4f(0.95f, 0.30f, 0.30f, 1) }, // +Z front
		{ { 1, 0, 3, 2 }, v4f(0.30f, 0.85f, 0.45f, 1) }, // -Z back
		{ { 0, 4, 7, 3 }, v4f(0.30f, 0.55f, 0.95f, 1) }, // -X left
		{ { 5, 1, 2, 6 }, v4f(0.95f, 0.80f, 0.25f, 1) }, // +X right
		{ { 3, 7, 6, 2 }, v4f(0.85f, 0.40f, 0.90f, 1) }, // +Y top
		{ { 0, 1, 5, 4 }, v4f(0.25f, 0.85f, 0.90f, 1) }, // -Y bottom
	};

	v4f *CubeVertexPositions = ArenaPushArrayZero(TemporaryArena(), v4f, 36);
	v4f *CubeVertexColours = ArenaPushArrayZero(TemporaryArena(), v4f, 36);
	int Count = 0;
	for (int FaceIndex = 0; FaceIndex < 6; FaceIndex++)
	{
		int *I = Faces[FaceIndex].I;
		v4f Colour = LinearFromSRGB(Faces[FaceIndex].Colour);
		int Tris[6] = { I[0], I[1], I[2], I[0], I[2], I[3] };
		for (int J = 0; J < 6; J++)
		{
			v3f P = Corners[Tris[J]];
			CubeVertexPositions[Count] = v4f(P.X, P.Y, P.Z, 1.0f);
			CubeVertexColours[Count]   = Colour;
			Count++;
		}
	}

	r_buffer CubeVertexPositionsBuffer = R_BufferPermanent(&App->Render, Str8("Cube Positions"), .SizeInBytes = sizeof(v4f) * 36, .Data = (void*)CubeVertexPositions);
	r_buffer CubeVertexColoursBuffer = R_BufferPermanent(&App->Render, Str8("Cube Colours"), .SizeInBytes = sizeof(v4f) * 36, .Data = (void*)CubeVertexColours);

	m44f Model = MulM44FM44F(M44FromYAngle((f32)App->Render.Frame / 100.0 * 0.9f),
	                         M44FromXAngle((f32)App->Render.Frame / 100.0 * 0.6f));
	m44f View  = M44FLookAt(v3f(0, 0, 3), v3f(0, 0, 0), v3f(0, 1, 0));
	f32 Aspect = (f32)ViewportSize.Y / (f32)ViewportSize.X;
	m44f Proj  = PerspectiveMetal(Aspect, 40.0f, 0.1f, 100.0f);
	m44f PVM   = MulM44FM44F(MulM44FM44F(Model, View), Proj);

	f32 ShapeY = H * 0.12f;
	f32 ShapeSize = 96.0f;
	f32 ShapeGap = 32.0f;

	f32 CardX0 = W * 0.10f - 30.0f;
	f32 CardX1 = W * 0.10f + (ShapeSize + ShapeGap) + ShapeSize * 2.0f + 30.0f;

	r_texture TestTexture = R_Texture(&App->Render, Str8("TestTexture"), .Size = v2i(400, 400), .Format = TF_RGBA16);
	r_texture TestDepth   = R_Texture(&App->Render, Str8("TestDepth"), .Size = v2i(400, 400), .Format = TF_DEPTH32);

	R_Pass(&App->Render, Str8("Cube Pass"), 
		.Depth = TestDepth, .DepthOp = LO_CLEAR, .ClearDepth = 1.0,
		.Colours[0] = TestTexture, .ColourOps[0] = LO_CLEAR, .ClearColour[0] = v4f(0, 0, 0, 0)
	)

	{
		R_BindBuffer(&App->Render, CubeVertexPositionsBuffer);
		R_BindBuffer(&App->Render, CubeVertexColoursBuffer);
		R_UniformM44F(&App->Render, Str8("PVMMatrix"), PVM);
		R_Draw(&App->Render, Str8("triangle2.metal"), 36, R_TRIANGLES);
	}

	r_texture Temp = R_Texture(&App->Render, Str8("TempTexture"), .Size = v2i(400, 400), .Format = TF_RGBA16);
	R_Pass(&App->Render, Str8("Cube Horizontal Blur"), .Colours[0] = Temp, .ColourOps[0] = LO_CLEAR, .ClearColour[0] = v4f(0, 0, 0, 0))
	{
		R_UniformV2F(&App->Render, Str8("Direction"), v2f(1, 0));
		R_UniformF32(&App->Render, Str8("Radius"), 10);
		R_BindTexture(&App->Render, Str8("Texture"), TestTexture);
		R_Draw(&App->Render, Str8("blur.metal"), 6, R_TRIANGLES);
	}

	R_Pass(&App->Render, Str8("Cube Vertical Blur"), .Colours[0] = TestTexture, .ColourOps[0] = LO_CLEAR, .ClearColour = v4f(0, 0, 0, 0))
	{
		R_UniformV2F(&App->Render, Str8("Direction"), v2f(0, 1));
		R_UniformF32(&App->Render, Str8("Radius"), 10);
		R_BindTexture(&App->Render, Str8("Texture"), Temp);
		R_Draw(&App->Render, Str8("blur.metal"), 6, R_TRIANGLES);
	}

	r_texture Screen = R_Texture(&App->Render, Str8("Screen Texture"),
		.Size = ViewportSize, .Format = TF_BGRA10_SRGB);


	R_Pass(&App->Render, Str8("Screen"), .Colours[0] = Screen, .ColourOps[0] = LO_CLEAR,
		.ClearColour[0] = v4f(0.001548f, 0.001548f, 0.002322f, 1.0f))
	{

		D_RoundedQuad(&App->Draw, v2f(200, 600), 400, 400, 15.0, 
			.OutlineThickness = 2.0f,
			.Colour = v4f(1.0, 1.0, 1.0, 1.0),
			.OutlineColour = v4f(1, 1, 1, 1),
		);

		v2f RenderPosition = v2f(200, 600);
		R_UniformM44F(&App->Render, Str8("PVMMatrix"), Ortho);
		R_UniformV2F(&App->Render, Str8("Position"), RenderPosition);
		R_UniformV2F(&App->Render, Str8("Size"), v2f(400, 400));
		R_BindTexture(&App->Render, Str8("Texture"), TestTexture);
		R_Draw(&App->Render, Str8("textured_quad.metal"), 6, R_TRIANGLES);

		D_RoundedQuad(&App->Draw, v2f(CardX0, ShapeY - 30.0f),
			CardX1 - CardX0, ShapeSize + 60.0f, 24.0f,
			.Colour = v4f(0.85f, 0.85f, 0.88f, 1));

		// Flat rounded quad.
		D_RoundedQuad(&App->Draw, v2f(W * 0.10f, ShapeY),
			ShapeSize, ShapeSize, 20.0f,
			.Colour = v4f(0.95f, 0.45f, 0.30f, 1),
			.Extrusion = 1.0,
			.ShadowOffset = v2f(8, 10), .ShadowBlur = 22.0f);

		// Capsule with an outline, also flat.
		D_Capsule(&App->Draw, v2f(W * 0.10f + (ShapeSize + ShapeGap), ShapeY),
			ShapeSize * 2.0f, ShapeSize * 0.55f,
			.Colour = v4f(0.35f, 0.85f, 0.55f, 1),
			.OutlineColour = v4f(1, 1, 1, 1),
			.OutlineThickness = 3.0f,
			.Extrusion = 1.0,
			.ShadowOffset = v2f(8, 10), .ShadowBlur = 22.0f);

		// Squircles with varying exponent. The middle one opts into a soft sheen
		// that tracks the cursor; the others stay flat for comparison.
		f32 SquircleStartX = W * 0.10f + (ShapeSize + ShapeGap) + ShapeSize * 2.0f + ShapeGap;
		f32 Exponents[3] = { 2.5f, 4.0f, 8.0f };
		for (int I = 0; I < (int)(sizeof(Exponents)/sizeof(Exponents[0])); I++)
		{
			f32 X = SquircleStartX + (ShapeSize + ShapeGap) * (f32)I;
			D_Squircle(&App->Draw, v2f(X, ShapeY),
				ShapeSize, ShapeSize, Exponents[I],
				.Colour = v4f(0.55f, 0.65f, 1.0f, 1),
				.OutlineColour = v4f(0.1f, 0.1f, 0.15f, 1),
				.OutlineThickness = 0.0f,
				.Extrusion = 1.0);
		}

		

		f32 ElapsedSec = OS_MilliSecondsFromTimeStamp(OS_TimeStampNow() - App->StartTime) * 0.001f;
		v4f CapsuleColours[2] = { v4f(1.0f, 0.4f, 0.0f, 1), v4f(0.8f, 0.35f, 0.0f, 1) };
		static b32 Clicked = False;
		static f32 ExtrusionTarget = 1.0;
		static f32 Extrusion = 1.0;

		Extrusion = LerpF32(Extrusion, ExtrusionTarget, 0.1);

		D_Str8(
			&App->Draw, Str8(NewFunction()),
			.Font = App->Font,
			.Position = v3f(W * 0.55f, H * 0.90f, 0),
			.HeightInPixels = 64.0f,
			.Colour = v4f(1, 1, 1, 1)
		);


		D_Str8(
			&App->Draw, Str8("adasdfadf"),
			.Font = App->Font,
			.Position = v3f(W * 0.55f, H * 0.85f, 0),
			.HeightInPixels = 64.0f,
			.Colour = v4f(1, 1, 1, 1)
		);

		// UI_Begin(&App->UI);
		// UI_PANEL(&App->UI, v2f(0, 0), v2f(ViewportSize.X/2.0, ViewportSize.Y/1.50))
		// {
		// 	UI_ROW(&App->UI, UI_LAYOUT_FIXED)
		// 	{
		// 		UI_FILL(&App->UI) UI_Button(&App->UI, Str8("A"));
		// 		UI_Button(&App->UI, Str8("Submit"));
		// 	}
		// 	str8 Elements[3] = {
		// 		Str8("Cat"), 
		// 		Str8("Dog"),
		// 		Str8("Meow"),
		// 	};
		// 	UI_ComboBox(&App->UI, Str8("ComboBox"), Elements, 3, 0);

		// 	UI_CollapsingTexture(&App->UI, Str8("Cube"), Str8("Cube widget"), TestTexture);

		// 	if(UI_Button(&App->UI, Str8("Some button")))
		// 	{
		// 		LogInfo("Clicked button");
		// 	}
		// }

		// UI_Layout(&App->UI);
		// UI_LayoutPositions(&App->UI, App->UI.Root, App->UI.Root->Position);

		// for(int I = 0; I < App->UI.FloatCount; I++)
		// {
		// 	ui_widget *Float = App->UI.FloatRoots[I];
		// 	UI_LayoutColumn(&App->UI, Float, App->UI.Draw->ViewportSize);
		// 	UI_LayoutPositions(&App->UI, Float, Float->Position);
		// 	UI_RenderWidgets(&App->UI, Float);
		// }

		// UI_End(&App->UI);
	}

	R_Update(&App->Render, OS_NextDrawable(&App->Window), Screen);

	OS_EndFrame();
}