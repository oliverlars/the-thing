#include <metal_stdlib>
using namespace metal;

struct vertex_output
{
	float4 Position [[position]];
	float2 UV;
};


vertex vertex_output VertexMain(uint VertexID [[vertex_id]])
{
	float2 Corner = float2((VertexID << 1) & 2, VertexID & 2);
	vertex_output Output;
	Output.Position = float4(Corner * 2.0 - 1.0, 0.0, 1.0);
	Output.UV = float2(Corner.x, 1.0 - Corner.y);
	return Output;
}

fragment float4 FragmentMain(vertex_output Input [[stage_in]],
                             texture2d<float> Source [[texture(0)]])
{
	constexpr sampler Sampler(mag_filter::linear, min_filter::linear);
	return Source.sample(Sampler, Input.UV);
}
