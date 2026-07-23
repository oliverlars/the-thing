#pragma once
#include <stddef.h>

#include "maths.h"

typedef enum r_texture_format {
	TF_RGBA16,
	TF_RGBA32,
	TF_DEPTH32,
	TF_BGRA10_SRGB,
} r_texture_format;

typedef u64 r_handle;

#define MAX_TRANSFORMS (4096)
#define MAX_SCISSORS   (4096)
#define MAX_TEXTURES   (128)

// NEW API BEGIN

typedef struct r_texture {
	v2i Size;
	u64 Handle;
} r_texture;

typedef struct r_buffer {
	u64 Handle;
	b32 Transient;   // true = per-frame (indexes the frame's transient list)
} r_buffer;

typedef struct r_shader {
	u64 Handle;
} r_shader;

typedef struct r_texture_options {
	r_texture_format Format;
	v2i Size;
} r_texture_options;


typedef struct r_buffer_options {
	s32 SizeInBytes;
	u8 *Data;
} r_buffer_options;

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

	int Frame;

	void *Backend;

	m44f TransformStack[MAX_TRANSFORMS];
	int TransformCount;

	v4i ScissorStack[MAX_SCISSORS];
	int ScissorCount;

	r_pass_options PassOptions;
	r_shader Shader;

	// We generate a list of uniform declarations here, .e.g 'float Time'
	// which is reset after each dispatch/draw
	str8_list UniformDeclarations;

	s32 UniformStartOffset;
	s32 UniformBytesCursor;
	s32 LargestUniformAlignment;

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

r_buffer R_BufferPermanentEx(render_context *R, str8 Name, r_buffer_options *Options);
r_buffer R_BufferFrameEx    (render_context *R, str8 Name, r_buffer_options *Options);
void    *R_BufferData(render_context *R, r_buffer Buffer);
#define R_BufferPermanent(Render, Name, ...) R_BufferPermanentEx(Render, Name, &(r_buffer_options){ __VA_ARGS__ })
#define R_BufferFrame(Render, Name, ...)     R_BufferFrameEx(Render, Name, &(r_buffer_options){ __VA_ARGS__ })

r_shader R_Shader(render_context *R, str8 Name);

void R_BindBuffer(render_context *R, r_buffer Buffer);
void R_BindTexture(render_context *R, r_texture Texture);
void R_BindShader(render_context *R, r_shader Shader);

void R_UniformF32(render_context *R, str8 Name, f32 Value);
void R_UniformM44F(render_context *R, str8 Name, m44f Value);
void R_UniformV2F(render_context *R, str8 Name, v2f Value);
void R_UniformV4F(render_context *R, str8 Name, v4f Value);
void R_UniformU32(render_context *R, str8 Name, u32 Value);

void R_DrawTriangles(render_context *R, str8 Shader, s32 Count);


// NEW API END


void R_TextureFromBytes(render_context *R, u8 *Bytes, s32 Width, s32 Height);

r_handle R_MakeTexture(render_context *R, v2i Size, r_texture_format Format);
void     R_ReleaseTexture(render_context *R, r_handle Texture);

void R_PushTransform(render_context *Render, m44f Transform);
void R_PopTransform(render_context *Render);

void R_PushTarget(render_context *Render, r_handle Texture);
void R_PopTarget(render_context *Render);

void R_PushScissor(render_context *Render, v4i Scissor);
void R_PopScissor(render_context *Render);
v4i R_CurrentScissor(render_context *Render);

m44f R_CurrentTransform(render_context *Render);


void R_SetLight(render_context *Render, v3f Position, v3f Colour, f32 Intensity, f32 Radius);
