StructuredBuffer<float4> g_Texture;

cbuffer g_OutputInfo : register (b0)
{
	uint2 g_InitialSeed;
	uint2 g_OutResolution;
};

struct VS_OUTPUT
{
	float2 TexCoord : TEXCOORD0;
	float4 Pos : SV_POSITION;	
};

//---------------------------------------------------------------

float3 ToneMapping(float3 color, float exposure)
{
	return float3(1,1,1) - exp( - color * exposure);
}


float4 main(VS_OUTPUT In) : SV_TARGET0
{
	float3 color = g_Texture[ (In.Pos.x - 0.5) + (g_OutResolution.y - (In.Pos.y - 0.5)) * g_OutResolution.x ].rgb;
	color = ToneMapping(color, 2);
	return float4(color, 1);
}

