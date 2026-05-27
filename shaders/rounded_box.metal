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

// 3D signed distance to the extruded solid. The 2D rounded-box is the
// cross-section; we intersect that infinite prism with a z-slab spanning
// [0, Th] (Th < 0 just mirrors below the plane) and round the rim by R so the
// top reads as a flat face rolling into a quarter-circle edge. Extrusion is
// the signed slab height Th, so +1 is raised, 0 flat, -1 recessed.
float Map(float2 P, float Z, float2 HalfSize, float Radius, float Th)
{
	float D2 = DistanceToRoundedBox(P, HalfSize, Radius);
	float R  = 0.5 * abs(Th);
	float Cz = 0.5 * Th;
	// iq extrusion with rim rounding R (slab half-height == R, so the edge is a
	// full quarter circle): w = (D2 + R, |Z - Cz| - R + R).
	float2 W = float2(D2 + R, abs(Z - Cz));
	return min(max(W.x, W.y), 0.0) + length(max(W, 0.0)) - R;
}

// Normal = normalize(gradient(Map)), sampled with the tetrahedron technique
// (4 evaluations). Because it comes straight from the distance field, the
// rim's true 3D slope falls out automatically — no hand-written derivatives.
float3 MapNormal(float2 P, float Z, float2 HalfSize, float Radius, float Th)
{
	const float E = 0.5;
	const float2 K = float2(1.0, -1.0);
	float3 N =
		K.xyy * Map(P + E * K.xy, Z + E * K.y, HalfSize, Radius, Th) +
		K.yyx * Map(P + E * K.yy, Z + E * K.x, HalfSize, Radius, Th) +
		K.yxy * Map(P + E * K.yx, Z + E * K.y, HalfSize, Radius, Th) +
		K.xxx * Map(P + E * K.xx, Z + E * K.x, HalfSize, Radius, Th);
	return normalize(N);
}

// Orthographic top-down sphere trace: march straight down the +z axis until we
// touch the solid, returning the surface height for this pixel.
float TraceSurface(float2 P, float2 HalfSize, float Radius, float Th)
{
	float ZStart = max(Th, 0.0) + 1.0;
	float ZStop  = min(Th, 0.0) - 1.0;
	float Z = ZStart;
	for (int i = 0; i < 48; ++i)
	{
		float D = Map(P, Z, HalfSize, Radius, Th);
		if (D < 0.001) break;
		Z -= max(D, 0.05);
		if (Z < ZStop) { Z = ZStop; break; }
	}
	return Z;
}

// Plain Blinn-Phong on a real position + normal.
float3 ApplyLighting(float3 BaseColour, float3 Position, float3 Normal,
                     float3 LightPos, float3 LightColour)
{
	float Strength = max(LightColour.r, max(LightColour.g, LightColour.b));
	if (Strength <= 0.0) return BaseColour;

	float3 L = normalize(LightPos - Position);
	float3 V = float3(0.0, 0.0, 1.0);
	float3 H = normalize(L + V);

	const float Ambient = 0.5;
	float Lambert = max(0.0, dot(Normal, L));
	float Spec    = pow(max(0.0, dot(Normal, H)), 32.0);

	return BaseColour * (Ambient + (1.0 - Ambient) * Lambert) + BaseColour * Spec * 0.4;
}

fragment float4 FragmentMain(vertex_output Input [[stage_in]],
                             constant uniforms &Uniforms [[buffer(1)]])
{
	float2 P = Input.LocalPos - Input.Centre;
	float Dist = DistanceToRoundedBox(P, Input.HalfSize, Input.Radius);

	// 2D distance still drives coverage and antialiasing.
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

	float Bevel = min(12.0, min(Input.HalfSize.x, Input.HalfSize.y));
	float Th    = Bevel * Input.Extrusion;

	float3 Normal;
	float  SurfaceZ;
	if (abs(Th) < 1e-3)
	{
		// No extrusion → flat plate facing the viewer.
		Normal   = float3(0.0, 0.0, 1.0);
		SurfaceZ = 0.0;
	}
	else
	{
		SurfaceZ = TraceSurface(P, Input.HalfSize, Input.Radius, Th);
		Normal   = MapNormal(P, SurfaceZ, Input.HalfSize, Input.Radius, Th);
	}

	float3 Position = float3(Input.LocalPos, SurfaceZ);
	float3 Lit = ApplyLighting(BaseColour, Position, Normal,
	                           Uniforms.LightPosition.xyz, Uniforms.LightColour.rgb);
	return float4(Lit, FillAlpha * BaseAlpha);
}
