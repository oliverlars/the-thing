#include <metal_stdlib>

using namespace metal;

// Apple-style squircle = superellipse: |x/a|^n + |y/b|^n = 1.
// n=2 ellipse, n=4 classic squircle, n≈5 iOS icon, n→∞ rectangle.

struct vertex_output
{
	float4 Position [[position]];
	float2 LocalPos;
	float4 Colour;
	float2 Centre;
	float2 HalfSize;
	float  Exponent;
	float4 OutlineColour;
	float  OutlineThickness;
	float  Extrusion;
};

struct vertex_input
{
	float4 Position         [[attribute(0)]];
	float4 Colour           [[attribute(1)]];
	float  Exponent         [[attribute(2)]];
	float2 Centre           [[attribute(3)]];
	float2 HalfSize         [[attribute(4)]];
	float4 OutlineColour    [[attribute(5)]];
	float  OutlineThickness [[attribute(6)]];
	float  Extrusion        [[attribute(7)]];
};

struct uniforms
{
	float4x4 PVMMatrix;
	float4   LightPosition;
	float4   LightColour;
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
	Output.Exponent         = Vertices[VertexID].Exponent;
	Output.OutlineColour    = Vertices[VertexID].OutlineColour;
	Output.OutlineThickness = Vertices[VertexID].OutlineThickness;
	Output.Extrusion        = Vertices[VertexID].Extrusion;
	return Output;
}

// Approximate 2D signed distance to the superellipse, in pixels. The raw field
// |x/a|^n+|y/b|^n-1 is not a distance — its iso-lines are scaled squircles — so
// we divide by the analytic gradient magnitude (a first-order distance
// estimate). Computed in closed form so it's evaluable at any point, which the
// 3D trace below needs.
float Squircle2D(float2 P, float2 HalfSize, float N)
{
	float n = clamp(N, 2.0, 32.0);
	float2 A = max(HalfSize, float2(1e-5));
	float2 U = abs(P) / A;
	float Ux = pow(U.x, n);
	float Uy = pow(U.y, n);
	float S  = Ux + Uy;
	float Field = pow(S, 1.0 / n) - 1.0;

	// dField/dP = S^(1/n - 1) * |u|^(n-1) * sign(P) / A
	float Sp = pow(max(S, 1e-12), 1.0 / n - 1.0);
	float2 Grad = Sp * float2(pow(U.x, n - 1.0) * sign(P.x) / A.x,
	                          pow(U.y, n - 1.0) * sign(P.y) / A.y);
	return Field / max(length(Grad), 1e-5);
}

// 3D signed distance to the extruded solid: the squircle cross-section,
// intersected with a z-slab [0, Th], rim rounded by R. Th is the signed height
// (+1 raised, 0 flat, -1 recessed).
float Map(float2 P, float Z, float2 HalfSize, float Exponent, float Th)
{
	float D2 = Squircle2D(P, HalfSize, Exponent);
	float R  = 0.5 * abs(Th);
	float Cz = 0.5 * Th;
	float2 W = float2(D2 + R, abs(Z - Cz));
	return min(max(W.x, W.y), 0.0) + length(max(W, 0.0)) - R;
}

// Normal = normalize(gradient(Map)), tetrahedron sampling (4 evaluations).
float3 MapNormal(float2 P, float Z, float2 HalfSize, float Exponent, float Th)
{
	const float E = 0.5;
	const float2 K = float2(1.0, -1.0);
	float3 N =
		K.xyy * Map(P + E * K.xy, Z + E * K.y, HalfSize, Exponent, Th) +
		K.yyx * Map(P + E * K.yy, Z + E * K.x, HalfSize, Exponent, Th) +
		K.yxy * Map(P + E * K.yx, Z + E * K.y, HalfSize, Exponent, Th) +
		K.xxx * Map(P + E * K.xx, Z + E * K.x, HalfSize, Exponent, Th);
	return normalize(N);
}

// Orthographic top-down sphere trace to the surface height for this pixel.
float TraceSurface(float2 P, float2 HalfSize, float Exponent, float Th)
{
	float ZStart = max(Th, 0.0) + 1.0;
	float ZStop  = min(Th, 0.0) - 1.0;
	float Z = ZStart;
	for (int i = 0; i < 48; ++i)
	{
		float D = Map(P, Z, HalfSize, Exponent, Th);
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
	float Dist = Squircle2D(P, Input.HalfSize, Input.Exponent);

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
		Normal   = float3(0.0, 0.0, 1.0);
		SurfaceZ = 0.0;
	}
	else
	{
		SurfaceZ = TraceSurface(P, Input.HalfSize, Input.Exponent, Th);
		Normal   = MapNormal(P, SurfaceZ, Input.HalfSize, Input.Exponent, Th);
	}

	float3 Position = float3(Input.LocalPos, SurfaceZ);
	float3 Lit = ApplyLighting(BaseColour, Position, Normal,
	                           Uniforms.LightPosition.xyz, Uniforms.LightColour.rgb);
	return float4(Lit, FillAlpha * BaseAlpha);
}
