#include <metal_stdlib>

using namespace metal;

struct vertex_output
{
	float4 Position [[position]];
	float4 Colour;
};

vertex vertex_output VertexMain(
	constant uniforms &Uniforms [[buffer(0)]],
	constant float4 *Positions [[buffer(1)]],
	constant float4 *Colours [[buffer(2)]],
	uint VertexID [[vertex_id]])
{
	vertex_output Output;
	Output.Position = Uniforms.PVMMatrix * Positions[VertexID];
	Output.Colour = Colours[VertexID];
	return Output;
}

FragmentMain2
{
	float4 Colour = Input.Colour;
	return Colour;
}
