#include <metal_stdlib>

using namespace metal;

struct vertex_output
{
	float4 Position [[position]];
	float4 Colour;
};

vertex vertex_output VertexMain(constant float4 *Positions [[buffer(0)]],
								uint VertexID [[vertex_id]])
{
	float4 Colours[3] =
	{
		float4(0, 1, 0, 1),
		float4(0, 1, 0, 1),
		float4(0, 1, 0, 1),
	};
	vertex_output Output;
	Output.Position = Positions[VertexID];
	Output.Colour = Colours[(VertexID + 1) % 3];
	return Output;
}

fragment float4 FragmentMain(vertex_output Input [[stage_in]])
{
	float4 Colour = Input.Colour;
	return Colour;
}
