struct VS_IN
{
	float2 Pos : POSITION;
};

struct VS_OUTPUT
{
	float2 TexCoord : TEXCOORD0;
	float4 Pos : SV_POSITION;
};


//---------------------------------------------------------------

VS_OUTPUT main(VS_IN In)
{
	VS_OUTPUT Out = (VS_OUTPUT)0;
	Out.TexCoord = In.Pos;
	Out.Pos = float4(In.Pos*2.0f + float2(-1.0f, -1.0f), 0.0f, 1.0f);
	return Out;
}

