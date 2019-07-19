cbuffer CBuffer0
{
	matrix WVPMatrix; //64 bytes
};

TextureCube cube0;
SamplerState sampler0;

struct VOut
{
	float4 position : SV_POSITION;
	float4 color	: COLOR;//Note the spelling of this and all instances below
	float3 texcoord : TEXCOORD;
};




VOut VShader(float4 position : POSITION)
{
	VOut output;
    
    output.texcoord = position.xyz;
	output.position = mul(WVPMatrix, position);
	output.color = 1;
	

	return output;
}

float4 PShader(float4 position : SV_POSITION, float4 color : COLOR, float3 texcoord : TEXCOORD) : SV_TARGET
{
	return color * cube0.Sample(sampler0, texcoord);
}
