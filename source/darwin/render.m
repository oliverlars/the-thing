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

#define R_TEXTURE_TABLE_SIZE 16384

typedef struct metal_context
{
	id<MTLDevice>        Device;
	id<MTLCommandQueue>  CommandQueue;
	CAMetalLayer        *Layer;

	metal_texture_entry Textures[R_TEXTURE_TABLE_SIZE];

	id<MTLRenderPipelineState> BlitPipeline;

	metal_buffer_entry Buffers[4096];

	id<MTLHeap>    FrameHeaps[FRAMES_IN_FLIGHT];
	id<MTLBuffer>  FrameUniforms;

	id<MTLBuffer>  FrameBuffers[256];
	int            FrameBufferCount;

	id<MTLHeap>    PermanentArena;

	id<MTLBuffer>  BindlessTexturesBuffer;

	metal_shader Shaders[4096];

	v2i Viewport;

	r_texture BoundTextures[64];
	// HACK(Oliver): This is kinda weird
	str8      BoundTextureNames[64];
	int BoundTexturesCount;

	r_buffer BoundBuffers[16];
	int BoundBuffersCount;
	id<MTLRenderCommandEncoder> ActiveRenderEncoder;

	id<MTLCommandBuffer>        CommandBuffer;

	dispatch_semaphore_t InFlightSemaphore;
}
metal_context;


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

metal_shader LoadMetalShader(render_context *R, u64 TimeStamp, str8 Name, str8 UniformStruct)
{
	metal_context *Metal = R->Backend;
	metal_shader Shader = {0};
	Shader.Name = Str8Copy(R->Arena, Name);
	Shader.TimeStamp = TimeStamp;
	str8 Path = Str8PushF(TemporaryArena(), "shaders/?S", Name);
	str8 SourceNoPrelude = OS_LoadEntireFileIntoMemory(TemporaryArena(), Path);
	str8_list SourceAndPrelude = {0};
	Str8ListAppend(TemporaryArena(), &SourceAndPrelude, UniformStruct);
	Str8ListAppend(TemporaryArena(), &SourceAndPrelude, Str8("\n"));
	Str8ListAppend(TemporaryArena(), &SourceAndPrelude, SourceNoPrelude);
	str8 Source = Str8ListJoin(TemporaryArena(), &SourceAndPrelude);
	Source = ResolveShaderIncludes(TemporaryArena(), Str8("shaders/"), Source);
	NSError *Error = nil;
	NSString *NSSource = [[NSString alloc] initWithBytes:Source.Bytes length:Source.Size encoding:NSUTF8StringEncoding];
	MTLCompileOptions *CompileOptions = [MTLCompileOptions new];
	CompileOptions.enableLogging = (getenv("GPU_LOGGING") != NULL);
	Shader.Library = [Metal->Device newLibraryWithSource:NSSource options:CompileOptions error:&Error];
	Shader.Hash = M_FNVHashString(M_FNV_HASH_START, Name);
	if (!Shader.Library)
	{
		LogError("Shader \"?S\" compile failed: ?cstr",
			Name,
			(char*)[Error.localizedDescription cStringUsingEncoding:NSUTF8StringEncoding]);
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
		case TF_DEPTH32: MetalFormat = MTLPixelFormatDepth32Float; break;
		case TF_BGRA10_SRGB: MetalFormat = MTLPixelFormatBGRA10_XR_sRGB; break;
	}
	MTLTextureDescriptor *Desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MetalFormat
		width:Size.X
		height:Size.Y
		mipmapped:NO
	];
	Desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
	Desc.storageMode = MTLStorageModeShared;
	NSError *Error = nil;
	id<MTLTexture> Texture = [Metal->Device newTextureWithDescriptor:Desc];
	Texture.label = [
		[NSString alloc] initWithBytes:Name.Bytes
		length:Name.Size encoding:NSUTF8StringEncoding
	];
	return Texture;
}

b32 TextureOptionsEqual(r_texture_options *A, r_texture_options *B)
{
	return A->Format == B->Format && A->Size.X == B->Size.X && A->Size.Y == B->Size.Y;
}

r_texture R_TextureEx(render_context *R, str8 Name, r_texture_options *Options)
{
	metal_context *Metal = R->Backend;
	u64 Hash = M_FNVHashString(M_FNV_HASH_START, Name);
	u64 Start = Hash % R_TEXTURE_TABLE_SIZE;

	// Linear probe outwards from the hashed slot, wrapping around the table.
	// A zero hash marks a slot that has never been claimed; nothing is ever
	// evicted, so the first empty slot is also the end of this name's chain.
	for(u64 Step = 0; Step < R_TEXTURE_TABLE_SIZE; Step++)
	{
		u64 Index = (Start + Step) % R_TEXTURE_TABLE_SIZE;
		metal_texture_entry *Entry = Metal->Textures + Index;

		if(Entry->Hash == 0)
		{
			Entry->Hash    = Hash;
			Entry->Options = Options[0];
			Entry->Texture = MakeMetalTexture(R, Name, Options->Size, Options->Format);
			return (r_texture){Options->Size, Index, Name};
		}

		if(Entry->Hash == Hash)
		{
			// Same name, different shape — the old texture is dropped here but
			// stays alive as long as in-flight command buffers reference it.
			if(!TextureOptionsEqual(&Entry->Options, Options))
			{
				Entry->Options = Options[0];
				Entry->Texture = MakeMetalTexture(R, Name, Options->Size, Options->Format);
			}
			return (r_texture){Options->Size, Index, Name};
		}
	}

	LogError("Texture table is full, cannot allocate ?S", Name);
	return (r_texture){0};
}

id<MTLBuffer> MakeMetalBuffer(render_context *R, str8 Name, r_buffer_options *Options)
{
	metal_context *Metal = R->Backend;
	id<MTLBuffer> Buffer;
	if(Options->Data)
	{
		Buffer = [Metal->Device newBufferWithBytes:Options->Data length:Options->SizeInBytes options:MTLResourceStorageModeShared];
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

r_buffer R_BufferPermanentEx(render_context *R, str8 Name, r_buffer_options *Options)
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

r_buffer R_BufferFrameEx(render_context *R, str8 Name, r_buffer_options *Options)
{
	metal_context *Metal = R->Backend;
	id<MTLHeap> Heap = Metal->FrameHeaps[R->Frame % FRAMES_IN_FLIGHT];
	id<MTLBuffer> Buffer = [Heap newBufferWithLength:Options->SizeInBytes options:MTLResourceStorageModeShared];
	Buffer.label = [[NSString alloc] initWithBytes:Name.Bytes length:Name.Size encoding:NSUTF8StringEncoding];
	if(Options->Data)
	{
		MemoryCopy(Buffer.contents, Options->Data, Options->SizeInBytes);
	}
	int Index = Metal->FrameBufferCount++;
	Metal->FrameBuffers[Index] = Buffer;
	return (r_buffer){ .Handle = Index, .Transient = True };
}

void *R_BufferData(render_context *R, r_buffer Buffer)
{
	metal_context *Metal = R->Backend;
	id<MTLBuffer> B = Buffer.Transient ? Metal->FrameBuffers[Buffer.Handle] : Metal->Buffers[Buffer.Handle].Buffer;
	return B.contents;
}

metal_shader *MetalShaderFromName(render_context *R, str8 Name, str8 UniformStruct)
{
	metal_context *Metal = R->Backend;
	u64 Hash = M_FNVHashString(M_FNV_HASH_START, Name);
	Hash = M_FNVHashString(Hash, UniformStruct);
	s32 Index = (Hash % 4096);
	metal_shader *Shader = Metal->Shaders + Index;
	u64 TimeStamp = OS_LastModified(Str8PushF(TemporaryArena(), "shaders/?S", Name));
	if(Shader->Hash == Hash)
	{
		if(TimeStamp > Shader->TimeStamp)
		{
			metal_shader New = LoadMetalShader(R, TimeStamp, Name, UniformStruct);
			if(New.Library) Shader[0] = New;
			else Shader->TimeStamp = TimeStamp;
		}
	}
	else if(Shader->Hash == 0)
	{
		Shader[0] = LoadMetalShader(R, TimeStamp, Name, UniformStruct);
		Shader->Hash = Hash;
	}
	else
	{
		b32 Found = False;
		for(int I = 1; I < 4096; I++)
		{
			if(Metal->Shaders[(Index + I) % 4096].Hash == Hash)
			{
				Shader = Metal->Shaders + (Index + I) % 4096;
				if(TimeStamp > Shader->TimeStamp)
				{
					metal_shader New = LoadMetalShader(R, TimeStamp, Name, UniformStruct);
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

void R_BindTexture(render_context *R, str8 BindName, r_texture Texture)
{
	metal_context *Metal = R->Backend;
	Metal->BoundTextures[Metal->BoundTexturesCount] = Texture;
	Metal->BoundTextureNames[Metal->BoundTexturesCount] = BindName;
	Metal->BoundTexturesCount++;
	R_UniformU32(R, Str8PushF(TemporaryArena(), "?SIndex", BindName), Texture.Handle);
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
		(u8*)Metal->FrameUniforms.contents + R->UniformBytesCursor,
		&Value,
		sizeof(f32)
	);
	R->UniformBytesCursor += sizeof(simd_float1);
}

void R_UniformM44F(render_context *R, str8 Name, m44f Value)
{
	Str8ListAppend(TemporaryArena(), &R->UniformDeclarations, Str8PushF(TemporaryArena(), "metal::float4x4 ?S", Name));
	metal_context *Metal = R->Backend;

	R->LargestUniformAlignment = Max(R->LargestUniformAlignment, alignof(simd_float4x4));
	R->UniformBytesCursor = AlignUp(R->UniformBytesCursor, alignof(simd_float4x4));
	MemoryCopy(
		(u8*)Metal->FrameUniforms.contents + R->UniformBytesCursor,
		&Value,
		sizeof(simd_float4x4)
	);
	R->UniformBytesCursor += sizeof(simd_float4x4);
}

void R_UniformV2F(render_context *R, str8 Name, v2f Value)
{
	Str8ListAppend(TemporaryArena(), &R->UniformDeclarations, Str8PushF(TemporaryArena(), "metal::float2 ?S", Name));
	metal_context *Metal = R->Backend;

	R->LargestUniformAlignment = Max(R->LargestUniformAlignment, alignof(simd_float2));
	R->UniformBytesCursor = AlignUp(R->UniformBytesCursor, alignof(simd_float2));
	MemoryCopy(
		(u8*)Metal->FrameUniforms.contents + R->UniformBytesCursor,
		&Value,
		sizeof(simd_float2)
	);
	R->UniformBytesCursor += sizeof(simd_float2);
}

void R_UniformV4F(render_context *R, str8 Name, v4f Value)
{
	Str8ListAppend(TemporaryArena(), &R->UniformDeclarations, Str8PushF(TemporaryArena(), "metal::float4 ?S", Name));
	metal_context *Metal = R->Backend;

	R->LargestUniformAlignment = Max(R->LargestUniformAlignment, alignof(simd_float4));
	R->UniformBytesCursor = AlignUp(R->UniformBytesCursor, alignof(simd_float4));
	simd_float4 Packed = { Value.X, Value.Y, Value.Z, Value.W };
	MemoryCopy(
		(u8*)Metal->FrameUniforms.contents + R->UniformBytesCursor,
		&Packed,
		sizeof(simd_float4)
	);
	R->UniformBytesCursor += sizeof(simd_float4);
}

void R_UniformU32(render_context *R, str8 Name, u32 Value)
{
	Str8ListAppend(TemporaryArena(), &R->UniformDeclarations, Str8PushF(TemporaryArena(), "uint ?S", Name));
	metal_context *Metal = R->Backend;

	R->LargestUniformAlignment = Max(R->LargestUniformAlignment, alignof(u32));
	R->UniformBytesCursor = AlignUp(R->UniformBytesCursor, alignof(u32));
	MemoryCopy(
		(u8*)Metal->FrameUniforms.contents + R->UniformBytesCursor,
		&Value,
		sizeof(u32)
	);
	R->UniformBytesCursor += sizeof(u32);
}

MTLPrimitiveType MetalPrimType(r_primitive_type PrimType)
{
	switch(PrimType)
	{
		case R_TRIANGLES: return MTLPrimitiveTypeTriangle;
		case R_LINES: return MTLPrimitiveTypeLine;
	}
	return 0;
}

void R_Draw(render_context *R, str8 ShaderName, s32 Count, r_primitive_type PrimType)
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
	if(Metal->Textures[R->PassOptions.Depth.Handle].Texture)
	{
		PD.depthAttachmentPixelFormat = Metal->Textures[R->PassOptions.Depth.Handle].Texture.pixelFormat;
	}

	NSError *Error = nil;
	id<MTLRenderPipelineState> Pipeline = [Metal->Device newRenderPipelineStateWithDescriptor:PD error:&Error];
	if (!Pipeline)
	{
		LogError("Pipeline state failed: ?cstr",
			(char*)[Error.localizedDescription cStringUsingEncoding:NSUTF8StringEncoding]);
		return;
	}
	[Metal->ActiveRenderEncoder setVertexBuffer:Metal->FrameUniforms   offset:R->UniformStartOffset atIndex:0];
	[Metal->ActiveRenderEncoder setFragmentBuffer:Metal->FrameUniforms offset:R->UniformStartOffset atIndex:0];
	for(int I = 0; I < Metal->BoundBuffersCount; I++)
	{
		r_buffer BB = Metal->BoundBuffers[I];
		id<MTLBuffer> Buffer = BB.Transient ? Metal->FrameBuffers[BB.Handle] : Metal->Buffers[BB.Handle].Buffer;
		[Metal->ActiveRenderEncoder setVertexBuffer:Buffer   offset:0 atIndex:1 + I];
		[Metal->ActiveRenderEncoder setFragmentBuffer:Buffer offset:0 atIndex:1 + I];
	}
	id<MTLHeap> FrameHeap = Metal->FrameHeaps[R->Frame % FRAMES_IN_FLIGHT];
	for(int I = 0; I < Metal->BoundTexturesCount; I++)
	{
		id<MTLTexture> Texture = Metal->Textures[Metal->BoundTextures[I].Handle].Texture;
		MTLResourceID *BTB = Metal->BindlessTexturesBuffer.contents;
		BTB[Metal->BoundTextures[I].Handle] = Texture.gpuResourceID;
		[Metal->ActiveRenderEncoder useResource: Texture usage: MTLResourceUsageRead stages:MTLRenderStageFragment];
	}
	[Metal->ActiveRenderEncoder setFragmentBuffer:Metal->BindlessTexturesBuffer offset:0 atIndex:1 + Metal->BoundBuffersCount];
	Metal->BoundBuffersCount = 0;
	Metal->BoundTexturesCount = 0;
	[Metal->ActiveRenderEncoder setRenderPipelineState:Pipeline];
	[Metal->ActiveRenderEncoder drawPrimitives:MetalPrimType(PrimType) vertexStart:0 vertexCount:Count];
	R->UniformDeclarations.First = 0;
	R->UniformDeclarations.Last = 0;
	if(R->LargestUniformAlignment > 0)
	{
		R->UniformBytesCursor = AlignUp(R->UniformBytesCursor, R->LargestUniformAlignment);
	}
	R->UniformStartOffset = R->UniformBytesCursor;
}

r_shader R_Shader(render_context *R, str8 Name)
{
	str8_list UniformStruct = {0};
	Str8ListAppend(TemporaryArena(), &UniformStruct, Str8("struct uniforms\n{ \n"));
	for(str8_list_node *Node = R->UniformDeclarations.First; Node; Node = Node->Next)
	{
		Str8ListAppend(TemporaryArena(), &UniformStruct, Node->String);
		Str8ListAppend(TemporaryArena(), &UniformStruct, Str8(";"));
	}
	Str8ListAppend(TemporaryArena(), &UniformStruct, Str8("};\n"));
	u64 LastModified = OS_LastModified(Str8PushF(TemporaryArena(), "shaders/?S", Name));
	metal_context *Metal = R->Backend;

	str8_list Prelude = {0};

	Str8ListAppend(TemporaryArena(), &Prelude, Str8ListJoin(TemporaryArena(), &UniformStruct));

	Str8ListAppendF(TemporaryArena(), &Prelude, "\n struct generic_args { metal::texture2d<float> Textures[[id(0)]][?u32];};\n", R_TEXTURE_TABLE_SIZE);

	for(int I = 0; I < Metal->BoundTexturesCount; I++)
	{
		Str8ListAppendF(TemporaryArena(), &Prelude, "\n#define ?S (Args.Textures[Uniforms.?SIndex])\n", Metal->BoundTextureNames[I], Metal->BoundTextureNames[I]);
	}

	Str8ListAppend(TemporaryArena(), &Prelude, Str8("\n#define FragmentMain2 "));
	Str8ListAppend(TemporaryArena(), &Prelude, Str8("fragment float4 FragmentMain(vertex_output Input [[stage_in]],"));

	Str8ListAppendF(TemporaryArena(), &Prelude, " device generic_args &Args [[buffer(1)]], ");
	Str8ListAppendF(TemporaryArena(), &Prelude, " constant uniforms &Uniforms [[buffer(0)]]");

	Str8ListAppend(TemporaryArena(), &Prelude, Str8(")"));

	return (r_shader){ MetalShaderFromName(R, Name,  Str8ListJoin(TemporaryArena(), &Prelude)) - Metal->Shaders };
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

	R->PassOptions = *Options;
	MTLRenderPassDescriptor *PassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];

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
	if(Metal->Textures[Options->Depth.Handle].Texture)
	{
		PassDescriptor.depthAttachment.texture = Metal->Textures[Options->Depth.Handle].Texture;
		PassDescriptor.depthAttachment.loadAction = MetalLoadAction(Options->DepthOp);
		PassDescriptor.depthAttachment.clearDepth = Options->ClearDepth;
		PassDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
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

void R_Init(render_context *R, m_arena *Arena, v2i Size, r_init_options Options)
{
	metal_context *M = ArenaPushArrayZero(Arena, metal_context, 1);
	R->Backend = M;
	R->Arena = Arena;

	M->Device = (__bridge id<MTLDevice>)Options.Device;
	M->Layer = (__bridge CAMetalLayer*)Options.Layer;
		MTLLogStateDescriptor *logDesc = [MTLLogStateDescriptor new];

	logDesc.level = MTLLogLevelDebug;
	logDesc.bufferSize = 64 * 1024;

	NSError *err = nil;
	MTLCommandQueueDescriptor *QueueDesc = [MTLCommandQueueDescriptor new];
	if(Options.Logging)
	{
		id<MTLLogState> Logger = [M->Device newLogStateWithDescriptor:logDesc
															error:&err];

		[Logger addLogHandler:^(NSString *SubSystem, NSString *Category,
								MTLLogLevel Level, NSString *Message) {
			NSLog(@"[GPU] %@", Message);
		}];
		QueueDesc.logState = Logger;
	}
	M->CommandQueue = [M->Device newCommandQueueWithDescriptor:QueueDesc];
	M->Viewport = Size;

	for (int Index = 0; Index < FRAMES_IN_FLIGHT; Index++)
	{
		MTLHeapDescriptor *HeapDesc = [[MTLHeapDescriptor alloc] init];
		HeapDesc.size        = MegaBytes(64);
		HeapDesc.storageMode = MTLStorageModeShared;
		HeapDesc.type        = MTLHeapTypeAutomatic;
		M->FrameHeaps[Index] = [M->Device newHeapWithDescriptor:HeapDesc];
	}

	{
		str8 BlitSource = LoadShaderSource(TemporaryArena(), Options.ShaderDir, "blit.metal");
		NSError *Error = nil;
		NSString *NSSource = [[NSString alloc] initWithBytes:BlitSource.Bytes length:BlitSource.Size encoding:NSUTF8StringEncoding];	
		MTLCompileOptions *CompileOptions = [MTLCompileOptions new];

		CompileOptions.enableLogging = (getenv("GPU_LOGGING") != NULL);
		id<MTLLibrary> Library = [M->Device newLibraryWithSource:NSSource options:CompileOptions error:&Error];
		if (!Library)
		{
			LogError("Blit shader compile failed: ?cstr",
				(char*)[Error.localizedDescription cStringUsingEncoding:NSUTF8StringEncoding]);
		}
		else
		{
			MTLRenderPipelineDescriptor *PD = [[MTLRenderPipelineDescriptor alloc] init];
			PD.vertexFunction   = [Library newFunctionWithName:@"VertexMain"];
			PD.fragmentFunction = [Library newFunctionWithName:@"FragmentMain"];
			PD.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA10_XR_sRGB;
			M->BlitPipeline = [M->Device newRenderPipelineStateWithDescriptor:PD error:&Error];
			if (!M->BlitPipeline)
			{
				LogError("Blit pipeline failed: ?cstr",
					(char*)[Error.localizedDescription cStringUsingEncoding:NSUTF8StringEncoding]);
			}
		}
	}

	MTLHeapDescriptor *HeapDesc = [[MTLHeapDescriptor alloc] init];
	HeapDesc.size        = MegaBytes(128);
	HeapDesc.storageMode = MTLStorageModeShared;
	HeapDesc.type        = MTLHeapTypeAutomatic;
	M->PermanentArena = [M->Device newHeapWithDescriptor:HeapDesc];

	M->BindlessTexturesBuffer = [M->PermanentArena newBufferWithLength:R_TEXTURE_TABLE_SIZE*8 options:MTLResourceStorageModeShared];
	assert(M->BindlessTexturesBuffer);
	M->InFlightSemaphore = dispatch_semaphore_create(FRAMES_IN_FLIGHT);
}

void R_BeginFrame(render_context *R, v2i ViewportSize)
{
	metal_context *M = R->Backend;
	M->Viewport = ViewportSize;

	R->UniformBytesCursor = 0;
	R->UniformStartOffset = 0;
	R->UniformDeclarations = (str8_list){0};
	R->LargestUniformAlignment = 0;

	dispatch_semaphore_wait(M->InFlightSemaphore, DISPATCH_TIME_FOREVER);

	int Slot = R->Frame % FRAMES_IN_FLIGHT;
	for (int I = 0; I < M->FrameBufferCount; I++) M->FrameBuffers[I] = nil;
	M->FrameBufferCount = 0;
	M->FrameUniforms = nil;

	M->CommandBuffer = [M->CommandQueue commandBuffer];

	M->FrameUniforms = [M->FrameHeaps[Slot] newBufferWithLength:MegaBytes(8) options:MTLResourceStorageModeShared];
	M->FrameUniforms.label = @"Frame Uniforms";
}

void R_Update(render_context *R, void *DrawableHandle, r_texture Source)
{
	metal_context *M = R->Backend;
	id<CAMetalDrawable> Drawable = (__bridge id<CAMetalDrawable>)DrawableHandle;

	if (Drawable)
	{
		MTLRenderPassDescriptor *BlitPass = [MTLRenderPassDescriptor renderPassDescriptor];
		BlitPass.colorAttachments[0].texture     = Drawable.texture;
		BlitPass.colorAttachments[0].loadAction  = MTLLoadActionDontCare;
		BlitPass.colorAttachments[0].storeAction = MTLStoreActionStore;
		id<MTLRenderCommandEncoder> BlitEncoder = [M->CommandBuffer renderCommandEncoderWithDescriptor:BlitPass];
		[BlitEncoder setRenderPipelineState:M->BlitPipeline];
		[BlitEncoder setFragmentTexture:M->Textures[Source.Handle].Texture atIndex:0];
		[BlitEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
		[BlitEncoder endEncoding];

		[M->CommandBuffer presentDrawable:Drawable];
	}

	__block dispatch_semaphore_t BlockSem = M->InFlightSemaphore;
	[M->CommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> _) {
		dispatch_semaphore_signal(BlockSem);
	}];
	[M->CommandBuffer commit];

	R->Frame++;
	R->TransformCount = 0;
	R->ScissorCount = 0;
}
