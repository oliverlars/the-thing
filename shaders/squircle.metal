#include "sdf.metal"

float Squircle2D(float2 P, float2 HalfSize, float N)
{
	float n = clamp(N, 2.0, 32.0);
	float2 A = max(HalfSize, float2(1e-5));
	float2 U = abs(P) / A;
	float S  = pow(U.x, n) + pow(U.y, n);
	float Field = pow(S, 1.0 / n) - 1.0;

	// dField/dP = S^(1/n - 1) * |u|^(n-1) * sign(P) / A
	float Sp = pow(max(S, 1e-12), 1.0 / n - 1.0);
	float2 Grad = Sp * float2(pow(U.x, n - 1.0) * sign(P.x) / A.x,
	                          pow(U.y, n - 1.0) * sign(P.y) / A.y);
	return Field / max(length(Grad), 1e-5);
}

fragment float4 FragmentMain(vertex_output Input [[stage_in]],
                             constant uniforms &Uniforms [[buffer(0)]])
{
	float2 P = Input.LocalPos - Uniforms.Centre;
	float Dist = Squircle2D(P, Uniforms.HalfSize, Uniforms.Param);
	if (Uniforms.ShadowBlur > 0.0) return ShadeShadow(Dist, Uniforms.ShadowBlur, Uniforms.Colour);
	return ShadeSDF(Dist, Input.LocalPos, Uniforms.HalfSize,
	                Uniforms.Colour, Uniforms.OutlineColour, Uniforms.OutlineThickness,
	                Uniforms.Extrusion, 8.0, Uniforms);
}
