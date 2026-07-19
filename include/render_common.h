#pragma once
#include <stddef.h>

#include "maths.h"

#define VERTEX_ATTRIBUTE_FIELDS                       \
	X(v4f, Position,         POSITION,           0) \
	X(v4f, Colour,           COLOUR,             1) \
	X(v2f, UV,               UV,                 2) \
	X(f32, PointSize,        POINT_SIZE,         3) \
	X(v2f, Start,            START,              4) \
	X(v2f, Control,          CONTROL,            5) \
	X(v2f, End,              END,                6) \
	X(f32, Thickness,        THICKNESS,          7) \
	X(u32, CurveStart,       CURVE_START,        8) \
	X(u32, CurveCount,       CURVE_COUNT,        9) \
	X(v2f, Centre,           CENTRE,            10) \
	X(v2f, HalfSize,         HALF_SIZE,         11) \
	X(v4f, OutlineColour,    OUTLINE_COLOUR,    12) \
	X(f32, OutlineThickness, OUTLINE_THICKNESS, 13) \
	X(f32, Extrusion,        EXTRUSION,         14) \
	X(v4f, CornerRadii,      CORNER_RADII,      15) \
	X(f32, ShadowBlur,       SHADOW_BLUR,       16)

#define X(T, NAME, CAPS_NAME, INDEX) T NAME;
typedef struct r_vertex { VERTEX_ATTRIBUTE_FIELDS } r_vertex;
#undef X

#define X(T, NAME, CAPS_NAME, INDEX) R_VA_##CAPS_NAME = 1 << INDEX,
typedef enum r_vertex_attributes { VERTEX_ATTRIBUTE_FIELDS } r_vertex_attributes;
#undef X

#define X(T, NAME, CAPS_NAME, INDEX) R_VA_##CAPS_NAME,
static u64 G_VertexAttributes[] = { VERTEX_ATTRIBUTE_FIELDS };
#undef X

#define X(T, NAME, CAPS_NAME, INDEX) sizeof(T),
static u64 G_VertexAttributeSizes[] = { VERTEX_ATTRIBUTE_FIELDS };
#undef X

#define X(T, NAME, CAPS_NAME, INDEX) offsetof(r_vertex, NAME),
static u64 G_VertexAttributeOffsets[] = { VERTEX_ATTRIBUTE_FIELDS };
#undef X

typedef enum r_command_type
{
	R_COMMAND_GLYPH,
	R_COMMAND_TRIANGLE,
	R_COMMAND_POINTS,
	R_COMMAND_LINES,
	R_COMMAND_BEZIER,
	R_COMMAND_ROUNDED_BOX,
	R_COMMAND_SQUIRCLE,
	R_COMMAND_TEXTURE,
} r_command_type;

typedef enum r_texture_format {
	TF_RGBA16,
	TF_RGBA32,
	RF_DEPTH32,
} r_texture_format;

typedef u64 r_handle;

typedef struct r_command
{
	r_command_type Type;

	r_vertex *Vertices;
	int VerticesCount;

	m44f Transform;
	v4i Scissor;

	r_vertex_attributes Attributes;

	struct r_command *Next;
}
r_command;

#define MAX_TRANSFORMS (4096)
#define MAX_SCISSORS   (4096)
#define MAX_TEXTURES   (128)

// NEW API BEGIN

typedef struct r_texture {
	u64 Handle;
} r_texture;

typedef struct r_shader {
	u64 Handle;
} r_shader;

typedef struct r_texture_options {
	r_texture_format Format;
	v2i Size;
} r_texture_options;

typedef enum r_load_operation {
	LO_LOAD,
	LO_CLEAR,
	LO_DISCARD,
} r_load_operation;

typedef struct r_pass_options {
	r_texture Colours[4];
	r_load_operation ColourOps[4];
	r_texture Depth;
	r_load_operation DepthOp;
	v4f ClearColour[4];
	v4f ClearDepth;
} r_pass_options;

typedef struct render_context
{
	m_arena *Arena;
	r_command *CommandsFirst;
	r_command *CommandsLast;

	int Frame;

	void *Backend;

	m44f TransformStack[MAX_TRANSFORMS];
	int TransformCount;

	v4i ScissorStack[MAX_SCISSORS];
	int ScissorCount;

	r_pass_options PassOptions;
	r_shader Shader;


	// Global point light. Position is in the same coordinate space as shape
	// positions (world / pixel space for the default ortho setup). Radius is
	// the half-falloff distance in those same units. Default colour is zero,
	// which makes shapes render as pure base colour with no lighting
	// contribution — call R_SetLight to enable.
	v3f LightPosition;
	v3f LightColour;
	f32 LightIntensity;
	f32 LightRadius;
}
render_context;

void R_PassBegin(render_context *R, str8 Name, r_pass_options *Options);
void R_PassEnd(render_context *R, str8 Name);
#define R_Pass(Renderer, Name, ...) DeferLoop(R_PassBegin(Renderer, Name, &(r_pass_options) { __VA_ARGS__ }), R_PassEnd(Renderer, Name))

void R_FrameBegin(render_context *R);
void R_FrameEnd(render_context *R);
#define R_Frame(Command, Name) DeferLoop(R_FrameBegin(Command, Name), R_FrameEnd(Command))

r_texture R_TextureEx(render_context *R, str8 Name, r_texture_options *Options);
#define R_Texture(Render, Name, ...) R_TextureEx(Render, Name, &(r_texture_options){ __VA_ARGS__ })

r_shader R_Shader(render_context *R, str8 Name);

void R_DrawTriangles(render_context *R, s32 Count);


// NEW API END


r_vertex_attributes R_AttributesFromCommandType(r_command_type CommandType);

void R_TextureFromBytes(render_context *R, u8 *Bytes, s32 Width, s32 Height);

r_handle R_MakeTexture(render_context *R, v2i Size, r_texture_format Format);
void     R_ReleaseTexture(render_context *R, r_handle Texture);

void R_PushCommand(render_context *R, r_command Command);

void R_PushTransform(render_context *Render, m44f Transform);
void R_PopTransform(render_context *Render);

void R_PushTarget(render_context *Render, r_handle Texture);
void R_PopTarget(render_context *Render);

void R_PushScissor(render_context *Render, v4i Scissor);
void R_PopScissor(render_context *Render);
v4i R_CurrentScissor(render_context *Render);

m44f R_CurrentTransform(render_context *Render);

void R_UpdateCurves(render_context *Render, v2f *CurvePoints, int CurvePointsCount, int CurvePointsSoFar);

void R_SetLight(render_context *Render, v3f Position, v3f Colour, f32 Intensity, f32 Radius);
