#include <metal_stdlib>

using namespace metal;

struct vertex_output
{
	float4 Position [[position]];
	float2 LocalPos;
};

template<typename T>
float dot2(T Value)
{
	return dot(Value, Value);
}

// https://iquilezles.org/articles/distfunctions2d/
float DistanceToSegment(float2 Position, float2 Start, float2 End)
{
	float2 EndToStart = End - Start;
	float2 PositionToStart = Position - Start;
	float H = clamp( dot(PositionToStart, EndToStart)/dot(EndToStart,EndToStart), 0.0, 1.0);
	return length(PositionToStart - H*EndToStart);
}

// https://iquilezles.org/articles/distfunctions2d/
float DistanceToBezier(float2 Pos, float2 A, float2 B, float2 C)
{
	float2 a = B - A;
	float2 b = A - 2.0*B + C;
	float2 c = a * 2.0;
	float2 d = A - Pos;
	float kk = 1.0/dot(b,b);
	float kx = kk * dot(a,b);
	float ky = kk * (2.0*dot(a,a)+dot(d,b)) / 3.0;
	float kz = kk * dot(d,a);
	float res = 0.0;
	float p = ky - kx*kx;
	float p3 = p*p*p;
	float q = kx*(2.0*kx*kx-3.0*ky) + kz;
	float h = q*q + 4.0*p3;
	if( h >= 0.0)
	{
		h = sqrt(h);
		float2 x = (float2(h,-h)-q)/2.0;
		float2 uv = sign(x)*pow(abs(x), float2(1.0/3.0));
		float t = clamp( uv.x+uv.y-kx, 0.0, 1.0 );
		res = dot2(d + (c + b*t)*t);
	}
	else
	{
		float z = sqrt(-p);
		float v = acos( q/(p*z*2.0) ) / 3.0;
		float m = cos(v);
		float n = sin(v)*1.732050808;
		float3  t = clamp(float3(m+m,-n-m,n-m)*z-kx,0.0,1.0);
		res = min( dot2(d+(c+b*t.x)*t.x),
				   dot2(d+(c+b*t.y)*t.y) );
	}
	return sqrt(res);
}

bool AreColinear(float2 A, float2 B, float2 C) {
	float2 AB = B - A;
	float2 AC = C - A;
	float Cross = AB.x * AC.y - AB.y * AC.x;
	return fabs(Cross) < 1.0;
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
	float Distance;
	if (AreColinear(Uniforms.Start, Uniforms.Control, Uniforms.End))
	{
		Distance = DistanceToSegment(Input.LocalPos, Uniforms.Start, Uniforms.End);
	}
	else
	{
		Distance = DistanceToBezier(Input.LocalPos, Uniforms.Start, Uniforms.Control, Uniforms.End);
	}

	float HalfThickness = 0.5 * Uniforms.Thickness;
	float AA = smoothstep(HalfThickness + HalfThickness, HalfThickness - HalfThickness, Distance);

	return float4(Uniforms.Colour.rgb, Uniforms.Colour.a * AA);
}
