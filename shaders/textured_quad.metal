#include <metal_stdlib>
using namespace metal;

struct vertex_output
{
	float4 Position [[position]];
	float2 UV;
};

vertex vertex_output VertexMain(constant uniforms &Uniforms [[buffer(0)]],
								uint VertexID [[vertex_id]])
{
	float2 BL = Uniforms.Position;
	float2 TR = Uniforms.Position + Uniforms.Size;
	float2 BR = Uniforms.Position + float2(Uniforms.Size.x, 0);
	float2 TL = Uniforms.Position + float2(0, Uniforms.Size.y);
	float2 Positions[6] = {
		BL,
		BR,
		TR,
	
		BL,
		TR,
		TL,
	};

	float2 UVs[6] = {
		float2(0, 0),
		float2(1, 0),
		float2(1, 1),

		float2(0, 0),
		float2(1, 1),
		float2(0, 1),
	};

	os_log_default.log_error("ABC\n");


	vertex_output Output;
	Output.Position = Uniforms.PVMMatrix * float4(Positions[VertexID], 1);
	Output.Position.z = 1.0;
	Output.UV = UVs[VertexID];

	return Output;
}

struct generic_args
{
	array<texture2d<float>, 4> Textures [[id(0)]];
};

fragment float4 FragmentMain(
	vertex_output Input [[stage_in]],
	texture2d<float> Texture [[texture(0)]],
	constant generic_args &Args [[buffer(1)]]
)
{
	sampler Sampler(mag_filter::linear, min_filter::linear);

	return Args.Textures[0].sample(Sampler, Input.UV);
}