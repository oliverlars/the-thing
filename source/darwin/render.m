#include <CoreGraphics/CoreGraphics.h>
#include <Metal/Metal.h>
#include <QuartzCore/QuartzCore.h>
#include <simd/simd.h>

#include <assert.h>
#include <stdalign.h>

#include "memory.h"
#include "misc.h"
#include "maths.h"
#include "render.h"
#include "log.h"
#include "os.h"
#include "render_common.h"
#include "strings.h"

// <complex.h> (via simd headers) defines I as _Complex_I — collides with any
// loop variable named I. Drop it.
#undef I

#define FRAMES_IN_FLIGHT 3

typedef struct metal_shader
{
	// LEGACY!!!! REMOVE THIS!!!!
	id<MTLRenderPipelineState> PipelineState;

	id<MTLLibrary> Library;
	id<MTLFunction> VertexEntry;
	id<MTLFunction> FragmentEntry;
	id<MTLFunction> ComputeEntry;
	u64 TimeStamp;
	u64 Hash;
	str8 Name;
} metal_shader;

typedef struct metal_texture_entry
{
	id<MTLTexture> Texture;
	r_texture_options Options;
	u64 Hash;
}
metal_texture_entry;

typedef struct metal_buffer_entry
{
	id<MTLBuffer> Buffer;
	r_buffer_options Options;
	u64 Hash;
}
metal_buffer_entry;

typedef struct metal_context
{
	id<MTLDevice>        Device;
	id<MTLCommandQueue>  CommandQueue;
	CAMetalLayer        *Layer;

	metal_shader TriangleShader;
	metal_shader GlyphShader;
	metal_shader PointShader;
	metal_shader LineShader;
	metal_shader BezierShader;
	metal_shader RoundedBoxShader;
	metal_shader SquircleShader;

	id<MTLDepthStencilState> DepthOn;   // LessEqual + write
	id<MTLDepthStencilState> DepthOff;  // Always + no write

	id<MTLBuffer>  VertexBuffers [FRAMES_IN_FLIGHT];
	id<MTLBuffer>  UniformBuffers[FRAMES_IN_FLIGHT];
	id<MTLBuffer>  CurveBuffers  [FRAMES_IN_FLIGHT];

	metal_texture_entry Textures[4096];
	int            TextureCount;
	int            TextureFreeList[4096];
	int            TextureFreeListCount;

	metal_buffer_entry Buffers[4096];

	id<MTLBuffer>  FrameArenas[FRAMES_IN_FLIGHT];

	// Single one for now, these should eventually chain if it runs out
	id<MTLHeap>    PermanentArena;

	metal_shader Shaders[4096];

	id<MTLTexture> DepthTexture;

	v2f *Curves;
	int  TotalCurvesSoFar;

	v2i Viewport;

	r_buffer BoundBuffers[16];
	int BoundBuffersCount;
	id<MTLRenderCommandEncoder> ActiveRenderEncoder;

	id<MTLCommandBuffer>        CommandBuffer;

	dispatch_semaphore_t InFlightSemaphore;
}
metal_context;

typedef struct uniforms
{
	m44f PVMMatrix;       // 64 bytes
	v4f  LightPosition;   // 16 bytes — xyz = position, w = falloff radius (same units)
	v4f  LightColour;
	f32  LightIntensity;
	f32  Pad[3];
} uniforms;

void R_UpdateCurves(render_context *Render, v2f *CurvePoints, int CurvePointsCount, int CurvePointsSoFar)
{
	metal_context *M = Render->Backend;
	if (!M->Curves)
	{
		M->Curves = ArenaPushArray(Render->Arena, v2f, MegaBytes(2));
	}
	MemoryCopy(M->Curves + CurvePointsSoFar, CurvePoints, CurvePointsCount * sizeof(v2f));
	M->TotalCurvesSoFar += CurvePointsCount;
}

void R_TextureFromBytes(render_context *R, u8 *Bytes, s32 Width, s32 Height)
{
	(void)R; (void)Bytes; (void)Width; (void)Height;
}

#define _METAL_TYPE_v4f Float4
#define _METAL_TYPE_v2f Float2
#define _METAL_TYPE_f32 Float
#define _METAL_TYPE_u32 UInt
#define MetalType(T) _METAL_TYPE_##T

#define _METAL_SIMD_v4f simd_float4
#define _METAL_SIMD_v2f simd_float2
#define _METAL_SIMD_f32 f32
#define _METAL_SIMD_u32 u32
#define MetalSimdType(T) _METAL_SIMD_##T

static str8 LoadShaderFile(m_arena *Arena, str8 ShaderDir, char *Name)
{
	str8 Path = Str8PushF(Arena, "?S/?cstr", ShaderDir, Name);
	return OS_LoadEntireFileIntoMemory(Arena, Path);
}

static str8 ResolveShaderIncludes(m_arena *Arena, str8 ShaderDir, str8 Source);

metal_shader LoadMetalShader(render_context *R, u64 TimeStamp, str8 Name)
{
	metal_context *Metal = R->Backend;
	metal_shader Shader = {0};
	Shader.Name = Str8Copy(R->Arena, Name);
	Shader.TimeStamp = TimeStamp;
	str8 Path = Str8PushF(TemporaryArena(), "shaders/?S", Name);
	str8 Source = OS_LoadEntireFileIntoMemory(TemporaryArena(), Path);
	Source = ResolveShaderIncludes(TemporaryArena(), Str8("shaders/"), Source);
	NSError *Error = nil;
	NSString *NSSource = [[NSString alloc] initWithBytes:Source.Bytes length:Source.Size encoding:NSUTF8StringEncoding];
	Shader.Library = [Metal->Device newLibraryWithSource:NSSource options:nil error:&Error];
	if (!Shader.Library)
	{
		LogError("Shader compile failed: ?cstr",
			(char*)[Error.localizedDescription cStringUsingEncoding:NSUTF8StringEncoding]);
		// Keep Name/TimeStamp so the cache doesn't retry (and re-log) every
		// lookup — the next reload happens when the file is modified again.
		return Shader;
	}

	Shader.VertexEntry   = [Shader.Library newFunctionWithName:@"VertexMain"];
	Shader.FragmentEntry = [Shader.Library newFunctionWithName:@"FragmentMain"];
	Shader.ComputeEntry  = [Shader.Library newFunctionWithName:@"ComputeMain"];
	return Shader;
}

// HACK(Oliver): This should eventually use the allocator system
id<MTLTexture> MakeMetalTexture(render_context *R, str8 Name, v2i Size, r_texture_format Format)
{
	metal_context *Metal = R->Backend;
	MTLPixelFormat MetalFormat;
	switch(Format)
	{
		case TF_RGBA16: MetalFormat = MTLPixelFormatRGBA16Float; break;
		case TF_RGBA32: MetalFormat = MTLPixelFormatRGBA32Float; break;
	}
	MTLTextureDescriptor *Desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MetalFormat
		width:Size.X
		height:Size.Y
		mipmapped:NO
	];
	Desc.usage = MTLTextureUsageRenderTarget;
	Desc.storageMode = MTLStorageModePrivate;
	id<MTLTexture> Texture = [Metal->Device newTextureWithDescriptor:Desc];
	Texture.label = [
		[NSString alloc] initWithBytes:Name.Bytes
		length:Name.Size encoding:NSUTF8StringEncoding
	];
	return Texture;
}

b32 TextureOptionsEqual(r_texture_options *A, r_texture_options *B)
{
	return A->Format == B->Format && A->Size.X == B->Size.X && A->Size.Y && B->Size.Y;
}

r_texture R_TextureEx(render_context *R, str8 Name, r_texture_options *Options)
{
	metal_context *Metal = R->Backend;
	u64 Hash = M_FNVHashString(M_FNV_HASH_START, Name);
	metal_texture_entry *Entry = Metal->Textures + (Hash % 4096);
	if(Entry->Hash == Hash)
	{
		if(!TextureOptionsEqual(&Entry->Options, Options))
		{
			Entry->Texture = nil;
			Entry->Texture = MakeMetalTexture(R, Name, Options->Size, Options->Format);
		}
		return (r_texture){Entry - Metal->Textures};
	}
	else if(Entry->Texture)
	{
		for(int I = 1; I < 4095; I++)
		{
			if(Entry[I].Hash == Hash)
			{
				if(!TextureOptionsEqual(&Entry->Options, Options))
				{
					Entry->Texture = nil;
					Entry->Texture = MakeMetalTexture(R, Name, Options->Size, Options->Format);
				}
				return (r_texture){Entry - Metal->Textures};
			}
		}
	}
	else
	{
		Entry->Texture = MakeMetalTexture(R, Name, Options->Size, Options->Format);
	}
	return (r_texture){Entry - Metal->Textures};
}

id<MTLBuffer> MakeMetalBuffer(render_context *R, str8 Name, r_buffer_options *Options)
{
	metal_context *Metal = R->Backend;
	id<MTLBuffer> Buffer;
	if(Options->Data)
	{
		Buffer = [Metal->Device newBufferWithBytes:Options->Data length:Options->SizeInBytes options:MTLResourceStorageModePrivate];
	}
	else
	{
		Buffer = [Metal->Device newBufferWithLength:Options->SizeInBytes options:MTLResourceStorageModePrivate];
	}

	Buffer.label = [
		[NSString alloc] initWithBytes:Name.Bytes
		length:Name.Size encoding:NSUTF8StringEncoding
	];
	return Buffer;
}

r_buffer R_BufferEx(render_context *R, str8 Name, r_buffer_options *Options)
{
	metal_context *Metal = R->Backend;
	u64 Hash = M_FNVHashString(M_FNV_HASH_START, Name);
	metal_buffer_entry *Entry = Metal->Buffers + (Hash % 4096);
	if(Entry->Hash == Hash)
	{
		return (r_buffer){Entry - Metal->Buffers};
	}
	else if(Entry->Buffer)
	{
		for(int I = 1; I < 4095; I++)
		{
			if(Entry[I].Hash == Hash)
			{
				return (r_buffer){Entry - Metal->Buffers};
			}
		}
	}
	else
	{
		Entry->Buffer = MakeMetalBuffer(R, Name, Options);
	}
	return (r_buffer){Entry - Metal->Buffers};
}

metal_shader *MetalShaderFromName(render_context *R, str8 Name)
{
	metal_context *Metal = R->Backend;
	u64 Hash = M_FNVHashString(M_FNV_HASH_START, Name);
	metal_shader *Shader = Metal->Shaders + (Hash % 4096);
	u64 TimeStamp = OS_LastModified(Str8PushF(TemporaryArena(), "shaders/?S", Name));
	if(Str8Equal(Shader->Name, Name))
	{
		if(TimeStamp > Shader->TimeStamp)
		{
			// Keep the last working shader if the reload failed to compile;
			// bump the timestamp so we only retry once the file changes again.
			metal_shader New = LoadMetalShader(R, TimeStamp, Name);
			if(New.Library) Shader[0] = New;
			else Shader->TimeStamp = TimeStamp;
		}
	}
	else if(!Shader->Name.Size)
	{
		Shader[0] = LoadMetalShader(R, TimeStamp, Name);
	}
	else
	{
		b32 Found = False;
		for(int I = 1; I < 4095; I++)
		{
			if(Str8Equal(Shader[I].Name, Name))
			{
				Shader = Shader + I;
				if(TimeStamp > Shader->TimeStamp)
				{
					metal_shader New = LoadMetalShader(R, TimeStamp, Name);
					if(New.Library) Shader[0] = New;
					else Shader->TimeStamp = TimeStamp;
				}
				break;
			}
		}
	}
	return Shader;
}

void R_BindBuffer(render_context *R, r_buffer Buffer)
{
	metal_context *Metal = R->Backend;
	Metal->BoundBuffers[Metal->BoundBuffersCount++] = Buffer;
}

void R_BindShader(render_context *R, r_shader Shader)
{
	R->Shader = Shader;
}

void R_UniformF32(render_context *R, str8 Name, f32 Value)
{
	Str8ListAppend(TemporaryArena(), &R->UniformDeclarations, Str8PushF(TemporaryArena(), "float ?S", Name));
	metal_context *Metal = R->Backend;

	R->LargestUniformAlignment = Max(R->LargestUniformAlignment, alignof(simd_float1));
	R->UniformBytesCursor = AlignUp(R->UniformBytesCursor, alignof(simd_float1));
	MemoryCopy(
		Metal->FrameArenas[R->Frame % FRAMES_IN_FLIGHT].contents + R->UniformBytesCursor,
		&Value,
		sizeof(f32)
	);
	R->UniformBytesCursor += sizeof(simd_float1);
}

void R_DrawTriangles(render_context *R, str8 ShaderName, s32 Count)
{
	metal_context *Metal = R->Backend;
	r_shader Shader = R_Shader(R, ShaderName);
	metal_shader *MetalShader = Metal->Shaders + Shader.Handle;
	MTLRenderPipelineDescriptor *PD = [[MTLRenderPipelineDescriptor alloc] init];
	PD.vertexFunction = MetalShader->VertexEntry;
	PD.fragmentFunction = MetalShader->FragmentEntry;
	PD.colorAttachments[0].pixelFormat = Metal->Textures[R->PassOptions.Colours[0].Handle].Texture.pixelFormat;
	PD.colorAttachments[0].blendingEnabled = YES;
	PD.colorAttachments[0].sourceRGBBlendFactor        = MTLBlendFactorSourceAlpha;
	PD.colorAttachments[0].destinationRGBBlendFactor   = MTLBlendFactorOneMinusSourceAlpha;
	PD.colorAttachments[0].sourceAlphaBlendFactor      = MTLBlendFactorSourceAlpha;
	PD.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	PD.depthAttachmentPixelFormat = Metal->Textures[R->PassOptions.Depth.Handle].Texture.pixelFormat;

	NSError *Error = nil;
	id<MTLRenderPipelineState> Pipeline = [Metal->Device newRenderPipelineStateWithDescriptor:PD error:&Error];
	if (!Pipeline)
	{
		LogError("Pipeline state failed: ?cstr",
			(char*)[Error.localizedDescription cStringUsingEncoding:NSUTF8StringEncoding]);
		return;
	}
	for(int I = 0; I < Metal->BoundBuffersCount; I++)
	{
		id<MTLBuffer> Buffer = Metal->Buffers[Metal->BoundBuffers[I].Handle].Buffer;
		[Metal->ActiveRenderEncoder setVertexBuffer:Buffer offset:0 atIndex:I];
	}
	Metal->BoundBuffersCount = 0;
	[Metal->ActiveRenderEncoder setRenderPipelineState:Pipeline];
	[Metal->ActiveRenderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:Count];
	R->UniformDeclarations.First = 0;
	R->UniformDeclarations.Last = 0;
	R->UniformBytesCursor = AlignUp(R->UniformBytesCursor, R->LargestUniformAlignment);
}

r_shader R_Shader(render_context *R, str8 Name)
{
	u64 LastModified = OS_LastModified(Str8PushF(TemporaryArena(), "shaders/?S", Name));
	metal_context *Metal = R->Backend;

	return (r_shader){ MetalShaderFromName(R, Name) - Metal->Shaders };
}

MTLLoadAction MetalLoadAction(r_load_operation Op)
{
	switch(Op)
	{
		case LO_LOAD:    return MTLLoadActionLoad;
		case LO_DISCARD: return MTLLoadActionDontCare;
		case LO_CLEAR:   return MTLLoadActionClear;
	}
}

MTLClearColor MetalClearColour(v4f Colour)
{
	MTLClearColor Result;
	Result.red = Colour.X;
	Result.green = Colour.Y;
	Result.blue = Colour.Z;
	Result.alpha = Colour.W;

	return Result;
}

void R_PassBegin(render_context *R, str8 Name, r_pass_options *Options)
{
	metal_context *Metal = R->Backend;
	// Draws inside the pass build their pipeline state (attachment pixel
	// formats) from this.
	R->PassOptions = *Options;
	MTLRenderPassDescriptor *PassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
	// Default storeAction is DontCare, which lets the GPU discard the pass's
	// tile memory instead of writing it back to the texture — store explicitly.
	PassDescriptor.colorAttachments[0].texture = Metal->Textures[Options->Colours[0].Handle].Texture;
	PassDescriptor.colorAttachments[0].loadAction = MetalLoadAction(Options->ColourOps[0]);
	PassDescriptor.colorAttachments[0].clearColor = MetalClearColour(Options->ClearColour[0]);
	PassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
	PassDescriptor.colorAttachments[1].texture = Metal->Textures[Options->Colours[1].Handle].Texture;
	PassDescriptor.colorAttachments[1].loadAction = MetalLoadAction(Options->ColourOps[1]);
	PassDescriptor.colorAttachments[1].clearColor = MetalClearColour(Options->ClearColour[1]);
	PassDescriptor.colorAttachments[1].storeAction = MTLStoreActionStore;
	PassDescriptor.colorAttachments[2].texture = Metal->Textures[Options->Colours[2].Handle].Texture;
	PassDescriptor.colorAttachments[2].loadAction = MetalLoadAction(Options->ColourOps[2]);
	PassDescriptor.colorAttachments[2].clearColor = MetalClearColour(Options->ClearColour[2]);
	PassDescriptor.colorAttachments[2].storeAction = MTLStoreActionStore;
	PassDescriptor.colorAttachments[3].texture = Metal->Textures[Options->Colours[3].Handle].Texture;
	PassDescriptor.colorAttachments[3].loadAction = MetalLoadAction(Options->ColourOps[3]);
	PassDescriptor.colorAttachments[3].clearColor = MetalClearColour(Options->ClearColour[3]);
	PassDescriptor.colorAttachments[3].storeAction = MTLStoreActionStore;
	if(Options->Depth.Handle)
	{
		PassDescriptor.depthAttachment.texture = Metal->Textures[Options->Depth.Handle].Texture;
		PassDescriptor.depthAttachment.loadAction = MetalLoadAction(Options->DepthOp);
	}

	Metal->ActiveRenderEncoder = [Metal->CommandBuffer renderCommandEncoderWithDescriptor:PassDescriptor];
	Metal->ActiveRenderEncoder.label = [
		[NSString alloc] initWithBytes:Name.Bytes
		length:Name.Size encoding:NSUTF8StringEncoding
	];
	[Metal->ActiveRenderEncoder setCullMode:MTLCullModeNone];

}

void R_PassEnd(render_context *R, str8 Name)
{
	metal_context *Metal = R->Backend;
	[Metal->ActiveRenderEncoder endEncoding];
	Metal->ActiveRenderEncoder = nil;
	R->PassOptions = (r_pass_options){0};
}

static int MTLAlignmentFromAttribute(r_vertex_attributes Attribute)
{
	int Index = CTZ_U64(Attribute);
	#define X(T, NAME, CAPS_NAME, INDEX) alignof(MetalSimdType(T)),
	int LUT[] = { VERTEX_ATTRIBUTE_FIELDS };
	#undef X
	return LUT[Index];
}

void R_ReleaseTexture(render_context *R, r_handle Texture)
{
	metal_context *Metal = R->Backend;
	for(int I = 0; I < Metal->TextureFreeListCount; I++)
	{
		if(Metal->TextureFreeList[I] == Texture) 
		{
			LogError("Texture '?s32' already freed", Texture);
			return;
		}
	}
	Metal->TextureFreeList[Metal->TextureFreeListCount++] = Texture;
}

static int MTLSizeFromAttribute(r_vertex_attributes Attribute)
{
	int Index = CTZ_U64(Attribute);
	#define X(T, NAME, CAPS_NAME, INDEX) sizeof(MetalSimdType(T)),
	int LUT[] = { VERTEX_ATTRIBUTE_FIELDS };
	#undef X
	return LUT[Index];
}

static MTLVertexFormat MTLFormatFromAttribute(r_vertex_attributes Attribute)
{
	int Index = CTZ_U64(Attribute);
	#define X(T, NAME, CAPS_NAME, INDEX) Concat(MTLVertexFormat, MetalType(T)),
	MTLVertexFormat LUT[] = { VERTEX_ATTRIBUTE_FIELDS };
	#undef X
	return LUT[Index];
}

static MTLVertexDescriptor *MakeVertexDescriptor(r_command_type CommandType)
{
	MTLVertexDescriptor *VD = [[MTLVertexDescriptor alloc] init];
	r_vertex_attributes Attributes = R_AttributesFromCommandType(CommandType);

	int Index = 0;
	int Offset = 0;
	int MaxAlignment = 0;
	for (int A = 0; A < (int)ArrayCount(G_VertexAttributes); A++)
	{
		if (Attributes & G_VertexAttributes[A])
		{
			int Alignment = MTLAlignmentFromAttribute(G_VertexAttributes[A]);
			MaxAlignment = Max(MaxAlignment, Alignment);
			Offset = Align(Offset, Alignment);
			VD.attributes[Index].format = MTLFormatFromAttribute(G_VertexAttributes[A]);
			VD.attributes[Index].offset = Offset;
			VD.attributes[Index].bufferIndex = 0;
			Index++;
			Offset += MTLSizeFromAttribute(G_VertexAttributes[A]);
		}
	}
	VD.layouts[0].stride = Align(Offset, MaxAlignment ? MaxAlignment : 16);
	return VD;
}

static int StrideFromAttributes(r_vertex_attributes Attributes)
{
	int Offset = 0;
	int MaxAlignment = 0;
	for (int A = 0; A < (int)ArrayCount(G_VertexAttributes); A++)
	{
		if (Attributes & G_VertexAttributes[A])
		{
			int Alignment = MTLAlignmentFromAttribute(G_VertexAttributes[A]);
			int Size = MTLSizeFromAttribute(G_VertexAttributes[A]);
			MaxAlignment = Max(MaxAlignment, Alignment);
			Offset = Align(Offset, Alignment);
			Offset += Size;
		}
	}
	return Align(Offset, MaxAlignment ? MaxAlignment : 16);
}

// Metal's newLibraryWithSource has no include search path, so we resolve local
// #include "file" directives ourselves: each is replaced by the (recursively
// resolved) contents of that file from the shader dir. #include <...> is left
// untouched for the Metal compiler.
static str8 ResolveShaderIncludes(m_arena *Arena, str8 ShaderDir, str8 Source)
{
	str8_list Parts = {0};
	s64 LineStart = 0;
	for (s64 I = 0; I <= Source.Size; I++)
	{
		b32 EndOfLine = (I == Source.Size) || (Source.Bytes[I] == '\n');
		if (!EndOfLine) continue;

		u8 *Bytes = Source.Bytes + LineStart;
		s64 LineSize = I - LineStart;

		s64 J = 0;
		while (J < LineSize && (Bytes[J] == ' ' || Bytes[J] == '\t')) J++;

		char *Marker = "#include \"";
		s64 MarkerSize = 10;
		str8 IncludeName = {0};
		if (LineSize - J >= MarkerSize && memcmp(Bytes + J, Marker, MarkerSize) == 0)
		{
			s64 NameStart = J + MarkerSize;
			s64 K = NameStart;
			while (K < LineSize && Bytes[K] != '"') K++;
			if (K < LineSize) IncludeName = (str8){ K - NameStart, Bytes + NameStart };
		}

		if (IncludeName.Size > 0)
		{
			char *NameCStr = CStrFromStr8(Arena, IncludeName);
			str8 Included = LoadShaderFile(Arena, ShaderDir, NameCStr);
			Str8ListAppend(Arena, &Parts, ResolveShaderIncludes(Arena, ShaderDir, Included));
			Str8ListAppend(Arena, &Parts, Str8("\n"));
		}
		else
		{
			// Re-emit the original line, including its trailing newline (if any).
			s64 EmitSize = (I < Source.Size) ? (LineSize + 1) : LineSize;
			Str8ListAppend(Arena, &Parts, (str8){ EmitSize, Bytes });
		}

		LineStart = I + 1;
	}
	return Str8ListJoin(Arena, &Parts);
}

static str8 LoadShaderSource(m_arena *Arena, str8 ShaderDir, char *Name)
{
	str8 Raw = LoadShaderFile(Arena, ShaderDir, Name);
	return ResolveShaderIncludes(Arena, ShaderDir, Raw);
}

static metal_shader MakeShader(metal_context *M, str8 Source, r_command_type Type,
                               MTLPixelFormat ColorFormat, MTLPixelFormat DepthFormat)
{
	metal_shader Shader = {0};

	NSError *Error = nil;
	NSString *NSSource = [[NSString alloc] initWithBytes:Source.Bytes length:Source.Size encoding:NSUTF8StringEncoding];
	Shader.Library = [M->Device newLibraryWithSource:NSSource options:nil error:&Error];
	if (!Shader.Library)
	{
		LogError("Shader compile failed: ?cstr",
			(char*)[Error.localizedDescription cStringUsingEncoding:NSUTF8StringEncoding]);
		return Shader;
	}

	id<MTLFunction> Vert = [Shader.Library newFunctionWithName:@"VertexMain"];
	id<MTLFunction> Frag = [Shader.Library newFunctionWithName:@"FragmentMain"];

	MTLRenderPipelineDescriptor *PD = [[MTLRenderPipelineDescriptor alloc] init];
	PD.vertexFunction = Vert;
	PD.fragmentFunction = Frag;
	PD.colorAttachments[0].pixelFormat = ColorFormat;
	PD.colorAttachments[0].blendingEnabled = YES;
	PD.colorAttachments[0].sourceRGBBlendFactor      = MTLBlendFactorSourceAlpha;
	PD.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	PD.colorAttachments[0].sourceAlphaBlendFactor      = MTLBlendFactorSourceAlpha;
	PD.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	PD.depthAttachmentPixelFormat = DepthFormat;
	PD.vertexDescriptor = MakeVertexDescriptor(Type);

	Shader.PipelineState = [M->Device newRenderPipelineStateWithDescriptor:PD error:&Error];
	if (!Shader.PipelineState)
	{
		LogError("Pipeline state failed: ?cstr",
			(char*)[Error.localizedDescription cStringUsingEncoding:NSUTF8StringEncoding]);
	}
	return Shader;
}

void R_Init(render_context *R, m_arena *Arena, v2i Size, r_init_options Options)
{
	metal_context *M = ArenaPushArrayZero(Arena, metal_context, 1);
	R->Backend = M;
	R->Arena = Arena;

	M->Device = (__bridge id<MTLDevice>)Options.Device;
	M->Layer = (__bridge CAMetalLayer*)Options.Layer;
	M->CommandQueue = [M->Device newCommandQueue];
	M->Viewport = Size;

	for (int Index = 0; Index < FRAMES_IN_FLIGHT; Index++)
	{
		M->VertexBuffers[Index]  = [M->Device newBufferWithLength:MegaBytes(64) options:MTLResourceStorageModeShared];
		M->UniformBuffers[Index] = [M->Device newBufferWithLength:MegaBytes(4)  options:MTLResourceStorageModeShared];
		M->CurveBuffers[Index]   = [M->Device newBufferWithLength:MegaBytes(16) options:MTLResourceStorageModeShared];
	}

	MTLPixelFormat Color = MTLPixelFormatBGRA10_XR_sRGB;
	MTLPixelFormat Depth = MTLPixelFormatDepth32Float;

	M->TriangleShader   = MakeShader(M, LoadShaderSource(TemporaryArena(), Options.ShaderDir, "triangle.metal"),    R_COMMAND_TRIANGLE,    Color, Depth);
	M->GlyphShader      = MakeShader(M, LoadShaderSource(TemporaryArena(), Options.ShaderDir, "glyph.metal"),       R_COMMAND_GLYPH,       Color, Depth);
	M->PointShader      = MakeShader(M, LoadShaderSource(TemporaryArena(), Options.ShaderDir, "point.metal"),       R_COMMAND_POINTS,      Color, Depth);
	M->LineShader       = MakeShader(M, LoadShaderSource(TemporaryArena(), Options.ShaderDir, "line.metal"),        R_COMMAND_LINES,       Color, Depth);
	M->BezierShader     = MakeShader(M, LoadShaderSource(TemporaryArena(), Options.ShaderDir, "bezier.metal"),      R_COMMAND_BEZIER,      Color, Depth);
	M->RoundedBoxShader = MakeShader(M, LoadShaderSource(TemporaryArena(), Options.ShaderDir, "rounded_box.metal"), R_COMMAND_ROUNDED_BOX, Color, Depth);
	M->SquircleShader   = MakeShader(M, LoadShaderSource(TemporaryArena(), Options.ShaderDir, "squircle.metal"),    R_COMMAND_SQUIRCLE,    Color, Depth);

	// Two depth states — triangles/points test+write depth, the SDF primitives
	// and glyphs disable depth so transparent edges don't punch holes in things
	// drawn behind them.
	{
		MTLDepthStencilDescriptor *D = [MTLDepthStencilDescriptor new];
		D.depthCompareFunction = MTLCompareFunctionLessEqual;
		D.depthWriteEnabled = YES;
		M->DepthOn = [M->Device newDepthStencilStateWithDescriptor:D];

		D = [MTLDepthStencilDescriptor new];
		D.depthCompareFunction = MTLCompareFunctionAlways;
		D.depthWriteEnabled = NO;
		M->DepthOff = [M->Device newDepthStencilStateWithDescriptor:D];
	}

	M->InFlightSemaphore = dispatch_semaphore_create(FRAMES_IN_FLIGHT);
}

void R_BeginFrame(render_context *R, v2i ViewportSize)
{
	metal_context *M = R->Backend;
	M->Viewport = ViewportSize;

	M->CommandBuffer = [M->CommandQueue commandBuffer];

	if (!M->DepthTexture ||
	    [M->DepthTexture width]  != (NSUInteger)ViewportSize.X ||
	    [M->DepthTexture height] != (NSUInteger)ViewportSize.Y)
	{
		MTLTextureDescriptor *Desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
		                                                                                width:ViewportSize.X
		                                                                               height:ViewportSize.Y
		                                                                            mipmapped:NO];
		Desc.usage = MTLTextureUsageRenderTarget;
		Desc.storageMode = MTLStorageModePrivate;
		M->DepthTexture = [M->Device newTextureWithDescriptor:Desc];
	}
}

static int CopyCommandVerticesToGPU(r_command *Command, u8 *Destination)
{
	r_vertex_attributes Attributes = Command->Attributes;
	int Stride = StrideFromAttributes(Attributes);

	u8 *Out = Destination;
	for (int V = 0; V < Command->VerticesCount; V++)
	{
		r_vertex *Vx = Command->Vertices + V;
		int Offset = 0;
		for (int A = 0; A < (int)ArrayCount(G_VertexAttributes); A++)
		{
			if (Attributes & G_VertexAttributes[A])
			{
				int Alignment = MTLAlignmentFromAttribute(G_VertexAttributes[A]);
				int Size = MTLSizeFromAttribute(G_VertexAttributes[A]);
				Offset = Align(Offset, Alignment);
				u8 *Src = (u8*)Vx + G_VertexAttributeOffsets[A];
				MemoryCopy(Out + Offset, Src, Size);
				Offset += Size;
			}
		}
		Out += Stride;
	}
	return Stride * Command->VerticesCount;
}

static MTLScissorRect ScissorFor(v4i Scissor, v2i Viewport)
{
	int X = Scissor.X;
	int Y = Viewport.Y - (Scissor.Y + Scissor.W);
	int W = Scissor.Z;
	int H = Scissor.W;
	if (X < 0) { W += X; X = 0; }
	if (Y < 0) { H += Y; Y = 0; }
	if (X + W > Viewport.X) W = Viewport.X - X;
	if (Y + H > Viewport.Y) H = Viewport.Y - Y;
	if (W < 0) W = 0;
	if (H < 0) H = 0;
	return (MTLScissorRect){ .x = X, .y = Y, .width = W, .height = H };
}

void R_Update(render_context *R, void *DrawableHandle)
{
	metal_context *M = R->Backend;
	id<CAMetalDrawable> Drawable = (__bridge id<CAMetalDrawable>)DrawableHandle;
	if (!Drawable) return;

	dispatch_semaphore_wait(M->InFlightSemaphore, DISPATCH_TIME_FOREVER);

	int ResourceIndex = R->Frame % FRAMES_IN_FLIGHT;

	if (M->TotalCurvesSoFar > 0)
	{
		MemoryCopy(M->CurveBuffers[ResourceIndex].contents,
		           M->Curves, M->TotalCurvesSoFar * sizeof(v2f));
	}

	// Stage 1: pack every command's vertex data into the frame's vertex
	// buffer, and stash each batch's (offset, count, uniform index) so the
	// single render pass below can issue draws back-to-back.
	typedef struct draw {
		r_command_type Type;
		v4i Scissor;
		int VertexOffset;     // bytes
		int VertexCount;
		int UniformIndex;
	} draw;

	m_scratch_arena Scratch = ScratchBegin(0, 0);
	int MaxDraws = 4096;
	draw *Draws = ArenaPushArray(Scratch.Arena, draw, MaxDraws);
	int DrawCount = 0;

	int VertexOffsetInBytes = 0;
	int CurrentBatchBytes = 0;
	int UniformIndex = 0;

	for (r_command *Command = R->CommandsFirst; Command; Command = Command->Next)
	{
		u8 *Dest = (u8*)M->VertexBuffers[ResourceIndex].contents + VertexOffsetInBytes + CurrentBatchBytes;
		CurrentBatchBytes += CopyCommandVerticesToGPU(Command, Dest);

		b32 EndOfBatch = !Command->Next
		              || Command->Next->Type != Command->Type
		              || !MemoryEqual(&Command->Transform, &Command->Next->Transform, sizeof(m44f));
		if (!EndOfBatch) continue;

		uniforms U = {
			.PVMMatrix     = Command->Transform,
			.LightPosition = v4f(R->LightPosition.X, R->LightPosition.Y, R->LightPosition.Z, R->LightRadius),
			.LightColour   = v4f(R->LightColour.X,   R->LightColour.Y,   R->LightColour.Z,   0),
			.LightIntensity = R->LightIntensity,
		};
		MemoryCopy((u8*)M->UniformBuffers[ResourceIndex].contents + UniformIndex * sizeof(uniforms),
		           &U, sizeof(U));

		int Stride = StrideFromAttributes(Command->Attributes);
		assert(DrawCount < MaxDraws);
		Draws[DrawCount++] = (draw){
			.Type         = Command->Type,
			.Scissor      = Command->Scissor,
			.VertexOffset = VertexOffsetInBytes,
			.VertexCount  = CurrentBatchBytes / Stride,
			.UniformIndex = UniformIndex,
		};

		VertexOffsetInBytes += CurrentBatchBytes;
		CurrentBatchBytes = 0;
		UniformIndex++;
	}

	// Stage 2: one render pass per frame. Clear once at the start, run every
	// draw inside it, present, commit.
	MTLRenderPassDescriptor *Pass = [MTLRenderPassDescriptor renderPassDescriptor];
	Pass.colorAttachments[0].texture     = Drawable.texture;
	Pass.colorAttachments[0].loadAction  = MTLLoadActionClear;
	Pass.colorAttachments[0].storeAction = MTLStoreActionStore;
	// sRGB target: the clear colour is treated as linear and encoded on store, so
	// linearise the sRGB values (0.02, 0.02, 0.03) to keep the same dark backdrop.
	Pass.colorAttachments[0].clearColor  = MTLClearColorMake(0.001548, 0.001548, 0.002322, 1.0);
	Pass.depthAttachment.texture     = M->DepthTexture;
	Pass.depthAttachment.loadAction  = MTLLoadActionClear;
	Pass.depthAttachment.storeAction = MTLStoreActionDontCare;
	Pass.depthAttachment.clearDepth  = 1.0;

	id<MTLRenderCommandEncoder> E = [M->CommandBuffer renderCommandEncoderWithDescriptor:Pass];
	[E setCullMode:MTLCullModeNone];

	for (int I = 0; I < DrawCount; I++)
	{
		draw *D = Draws + I;

		metal_shader *Shader = NULL;
		b32 WriteDepth = NO;
		b32 NeedsCurves = NO;
		switch (D->Type)
		{
		case R_COMMAND_TRIANGLE:    Shader = &M->TriangleShader;   WriteDepth = YES; break;
		case R_COMMAND_GLYPH:       Shader = &M->GlyphShader;      NeedsCurves = YES; break;
		case R_COMMAND_POINTS:      Shader = &M->PointShader;      WriteDepth = YES; break;
		case R_COMMAND_LINES:       Shader = &M->LineShader;       break;
		case R_COMMAND_BEZIER:      Shader = &M->BezierShader;     break;
		case R_COMMAND_ROUNDED_BOX: Shader = &M->RoundedBoxShader; break;
		case R_COMMAND_SQUIRCLE:    Shader = &M->SquircleShader;   break;
		}

		[E setRenderPipelineState:Shader->PipelineState];
		[E setDepthStencilState:(WriteDepth ? M->DepthOn : M->DepthOff)];
		[E setScissorRect:ScissorFor(D->Scissor, M->Viewport)];
		[E setVertexBuffer:M->VertexBuffers[ResourceIndex]    offset:D->VertexOffset                    atIndex:0];
		[E setVertexBuffer:M->UniformBuffers[ResourceIndex]   offset:D->UniformIndex * sizeof(uniforms) atIndex:1];
		// Fragment shaders for the lit SDF shapes also read uniforms (light
		// pos/colour). Vertex and fragment stages have independent buffer
		// tables, so this binding is required on top of the vertex one above.
		[E setFragmentBuffer:M->UniformBuffers[ResourceIndex] offset:D->UniformIndex * sizeof(uniforms) atIndex:1];
		if (NeedsCurves)
		{
			[E setFragmentBuffer:M->CurveBuffers[ResourceIndex] offset:0 atIndex:0];
		}
		[E drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:D->VertexCount];
	}

	[E endEncoding];

	[M->CommandBuffer presentDrawable:Drawable];

	__block dispatch_semaphore_t BlockSem = M->InFlightSemaphore;
	[M->CommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> _) {
		dispatch_semaphore_signal(BlockSem);
	}];
	[M->CommandBuffer commit];

	ScratchEnd(Scratch);

	R->CommandsFirst = NULL;
	R->CommandsLast = NULL;
	R->Frame++;
	R->TransformCount = 0;
	R->ScissorCount = 0;
}
