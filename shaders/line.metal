#include <metal_stdlib>

using namespace metal;

struct vertex_output
{
	float4 Position [[position]];
	float2 LocalPos;
};

// https://iquilezles.org/articles/distfunctions2d/
float DistanceToSegment(float2 Position, float2 Start, float2 End)
{
	float2 EndToStart = End - Start;
	float2 PositionToStart = Position - Start;
	float H = clamp( dot(PositionToStart, EndToStart)/dot(EndToStart,EndToStart), 0.0, 1.0);
	return length(PositionToStart - H*EndToStart);
}

vertex vertex_output VertexMain(constant uniforms &Uniforms [[buffer(0)]],
                                uint VertexID [[vertex_id]])
{
	float2 Min = Uniforms.QuadMin;
	float2 Max = Uniforms.QuadMax;
	float2 Corners[6] = {
		float2(Min.x, Min.y), float2(Max.x, Min.y), float2(Max.x, Max.y),
		float2(Min.x, Min.y), float2(Min.x, Max.y), float2(Max.x, Max.y),
	};
	vertex_output Output;
	Output.Position = Uniforms.PVMMatrix * float4(Corners[VertexID], 0.0, 1.0);
	Output.LocalPos = Corners[VertexID];
	return Output;
}

fragment float4 FragmentMain(vertex_output Input [[stage_in]],
                             constant uniforms &Uniforms [[buffer(0)]])
{
	float HalfThickness = 0.5 * Uniforms.Thickness;
	float Distance = DistanceToSegment(Input.LocalPos, Uniforms.Start, Uniforms.End) - HalfThickness;
	float AA = smoothstep(HalfThickness + HalfThickness, HalfThickness - HalfThickness, Distance);

	return float4(Uniforms.Colour.rgb, float(AA)*Uniforms.Colour.a);
}
