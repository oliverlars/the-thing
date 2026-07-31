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

FragmentMain2
{
	sampler Sampler(mag_filter::linear, min_filter::linear);

	return Texture.sample(Sampler, Input.UV);
}