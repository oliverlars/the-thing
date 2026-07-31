#include <metal_stdlib>


using namespace metal;

struct vertex_output
{
	float4 Position [[position]];
};

vertex vertex_output VertexMain(
	constant uniforms &Uniforms [[buffer(0)]],
	uint VertexID [[vertex_id]]
)
{
	float2 Positions[6] = {
		float2(-1, -1),
		float2(+1, -1),
		float2(+1, +1),

		float2(-1, -1),
		float2(+1, +1),
		float2(-1, +1),
	};

	vertex_output Output;
	Output.Position = float4(Positions[VertexID], 0, 1);
	return Output;
}

FragmentMain2
{
	float4 Average = float4(0.0);
	int Radius = Uniforms.Radius;
	uint2 Size = uint2(Texture.get_width(), Texture.get_height());
	int Samples = 0;
	for(int I = -Radius; I <= +Radius; I++)
	{
		uint2 Index = uint2(Input.Position.xy) + uint2(I, I) * uint2(Uniforms.Direction);
		if(any(Index < 0) || any(Index >= Size)) continue;
		Average += Texture.read(Index);
		Samples += 1;
	}
	Average /= float(Samples);

	return Average;
}