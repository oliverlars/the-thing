#include "sdf.metal"

// https://iquilezles.org/articles/distfunctions2d/
float DistanceToRoundedBox(float2 P, float2 HalfSize, float Radius)
{
	float ClampedRadius = min(Radius, min(HalfSize.x, HalfSize.y));
	float2 Q = abs(P) - HalfSize + ClampedRadius;
	return min(max(Q.x, Q.y), 0.0) + length(max(Q, 0.0)) - ClampedRadius;
}

fragment float4 FragmentMain(vertex_output Input [[stage_in]],
                             constant uniforms &Uniforms [[buffer(1)]])
{
	float2 P = Input.LocalPos - Input.Centre;
	float Dist = DistanceToRoundedBox(P, Input.HalfSize, Input.Param);
	return ShadeSDF(Dist, Input.LocalPos, Input.HalfSize,
	                Input.Colour, Input.OutlineColour, Input.OutlineThickness,
	                Input.Extrusion, 10.0, Uniforms);
}
