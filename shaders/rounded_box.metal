#include <metal_stdlib>

using namespace metal;

struct vertex_output
{
	float4 Position [[position]];
	float2 LocalPos;
	float4 Colour;
	float2 Centre;
	float2 HalfSize;
	float  Radius;
	float4 OutlineColour;
	float  OutlineThickness;
	float  Extrusion;
};

// Attribute indices follow enum bit order in r_vertex_attributes, not
// declaration order.
struct vertex_input
{
	float4 Position         [[attribute(0)]];
	float4 Colour           [[attribute(1)]];
	float  Radius           [[attribute(2)]];
	float2 Centre           [[attribute(3)]];
	float2 HalfSize         [[attribute(4)]];
	float4 OutlineColour    [[attribute(5)]];
	float  OutlineThickness [[attribute(6)]];
	float  Extrusion        [[attribute(7)]];
};

struct uniforms
{
	float4x4 PVMMatrix;
	float4   LightPosition;   // xyz = world-space position of point light
	float4   LightColour;     // rgb = colour * intensity; zero = lighting off
};

vertex vertex_output VertexMain(constant vertex_input *Vertices [[buffer(0)]],
                                constant uniforms &Uniforms [[buffer(1)]],
                                uint VertexID [[vertex_id]])
{
	vertex_output Output;
	Output.Position         = Uniforms.PVMMatrix * Vertices[VertexID].Position;
	Output.LocalPos         = Vertices[VertexID].Position.xy;
	Output.Colour           = Vertices[VertexID].Colour;
	Output.Centre           = Vertices[VertexID].Centre;
	Output.HalfSize         = Vertices[VertexID].HalfSize;
	Output.Radius           = Vertices[VertexID].Radius;
	Output.OutlineColour    = Vertices[VertexID].OutlineColour;
	Output.OutlineThickness = Vertices[VertexID].OutlineThickness;
	Output.Extrusion        = Vertices[VertexID].Extrusion;
	return Output;
}

// https://iquilezles.org/articles/distfunctions2d/
float DistanceToRoundedBox(float2 P, float2 HalfSize, float Radius)
{
	float ClampedRadius = min(Radius, min(HalfSize.x, HalfSize.y));
	float2 Q = abs(P) - HalfSize + ClampedRadius;
	return min(max(Q.x, Q.y), 0.0) + length(max(Q, 0.0)) - ClampedRadius;
}

// Plain Blinn-Phong on a position + normal.
float3 ApplyLighting(float3 BaseColour, float3 Position, float3 Normal,
                     float3 LightPos, float3 LightColour)
{
	float Strength = max(LightColour.r, max(LightColour.g, LightColour.b));
	if (Strength <= 0.0) return BaseColour;

	float3 L = normalize(LightPos - Position);
	float3 V = float3(0.0, 0.0, 1.0);
	float3 H = normalize(L + V);

	const float Ambient = 0.35;
	float Lambert = max(0.0, dot(Normal, L));
	float Spec    = pow(max(0.0, dot(Normal, H)), 32.0);

	return BaseColour * (Ambient + (1.0 - Ambient) * Lambert) + BaseColour * Spec * 0.4;
}

fragment float4 FragmentMain(vertex_output Input [[stage_in]],
                             constant uniforms &Uniforms [[buffer(1)]])
{
	float2 P = Input.LocalPos - Input.Centre;
	float Dist = DistanceToRoundedBox(P, Input.HalfSize, Input.Radius);

	// 2D distance drives coverage and antialiasing.
	float FillAlpha = clamp(0.5 - Dist / max(fwidth(Dist), 1e-5), 0.0, 1.0);
	if (FillAlpha <= 0.0) discard_fragment();

	float3 BaseColour;
	float  BaseAlpha;
	if (Input.OutlineThickness > 0.0)
	{
		float PW = max(fwidth(Dist), 1e-5);
		float InteriorAlpha = clamp(0.5 - (Dist + Input.OutlineThickness) / PW, 0.0, 1.0);
		BaseColour = mix(Input.OutlineColour.rgb, Input.Colour.rgb, InteriorAlpha);
		BaseAlpha  = mix(Input.OutlineColour.a,   Input.Colour.a,   InteriorAlpha);
	}
	else
	{
		BaseColour = Input.Colour.rgb;
		BaseAlpha  = Input.Colour.a;
	}

	float3 Lit;
	if (abs(Input.Extrusion) < 1e-3)
	{
		// Fully flat: no bevel and no per-pixel lighting. A flat normal under a
		// nearby point light still gets a grazing-falloff gradient that reads as a
		// faint dome, so for Extrusion 0 we skip lighting and emit the raw colour.
		Lit = BaseColour;
	}
	else
	{
		// Fake-3D normal map from the 2D field: treat the band Dist in [-Bevel, 0]
		// as a quarter-circle rolling from the flat top (normal +z) down to the
		// silhouette (normal horizontal). No raymarch — the distance field is
		// turned straight into a normal. Extrusion scales the tilt and signs the
		// roll direction (+ raised lip, - recessed wall).
		float Bevel = min(10.0, min(Input.HalfSize.x, Input.HalfSize.y) * 0.5);

		// SDF gradient points outward (toward increasing distance / the edge).
		float2 Grad    = float2(dfdx(Dist), -dfdy(Dist));
		float2 GradDir = Grad / max(length(Grad), 1e-5);

		float T       = clamp(-Dist / Bevel, 0.0, 1.0); // 0 at silhouette, 1 on flat top
		float Angle   = T * (3.14159265 * 0.5);
		float SlopeXY = cos(Angle);                     // 1 at edge → 0 on flat top
		float SlopeZ  = sin(Angle);                     // 0 at edge → 1 on flat top

		float3 Tilted = float3(sign(Input.Extrusion) * GradDir * SlopeXY, SlopeZ);
		float3 Normal = normalize(mix(float3(0.0, 0.0, 1.0), Tilted, abs(Input.Extrusion)));
		float  Height = Input.Extrusion * SlopeZ * Bevel;

		float3 Position = float3(Input.LocalPos, Height);
		Lit = ApplyLighting(BaseColour, Position, Normal,
		                    Uniforms.LightPosition.xyz, Uniforms.LightColour.rgb);
	}
	return float4(Lit, FillAlpha * BaseAlpha);
}
