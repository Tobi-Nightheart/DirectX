cbuffer CBuffer0
{
    matrix WVPMatrix; //64 bytes
    matrix world; //64	
};

cbuffer Light
{
    float4 EyePos;  //16
    float4 DirToLight;
    float4 DirLightColor;
    float4 AmbientDown;
    float4 AmbientRange;

};

struct Material
{
    float3 normal;
    float4 diffuseColor;
    float specExp;
    float specIntensity;
};


Texture2D texture0;
SamplerState sampler0;
struct VIn
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
};

struct VOut
{
	float4 position : SV_POSITION;
	float4 color	: COLOR;//Note the spelling of this and all instances below
	float2 texcoord : TEXCOORD;
	float3 normal : NORMAL;
};

//Helper functions
float4 CalcAmbient(float3 normal, float4 color)
{
    //convert from [-1|1] to [0|1]
    float up = normal.y * 0.5f + 0.5f;
    //calculate and apply ambient value
    float3 Ambient = (float3) (AmbientDown + up * AmbientRange);
    return float4(Ambient * color.rgb, color.a);
}

float4 CalcDirectional(float3 position, Material mat)
{
    //phong diffuse
    float NDotL = dot(DirLightColor.xyz, mat.normal);
    float3 finalColor = DirLightColor.rgb * saturate(NDotL);

    //Blinn specular
    float3 ToEye = EyePos.xyz - position;
    ToEye = normalize(ToEye);
    float3 HalfWay = normalize(ToEye + DirToLight.xyz);
    float NDotH = saturate(dot(HalfWay, mat.normal));
    finalColor += DirLightColor.rgb * pow(NDotH, mat.specExp) * mat.specIntensity;

    return float4(finalColor * mat.diffuseColor.rbg, 0.0f);
}

float4 VShaderOpaque(float4 position : POSITION) : SV_POSITION
{
    return mul(WVPMatrix, position);    
}

VOut VShader(VIn vin)
{
	VOut output;
    float4 default_color = {1.0f, 1.0f, 1.0f, 1.0f};
    output.position = mul(WVPMatrix, vin.position);
    output.texcoord = vin.texcoord;
    output.normal = (float3) mul(vin.normal, (float3x3) world);
    output.color = default_color;
    
	return output;
}

float4 PShader(float4 position : SV_POSITION, float4 color : COLOR, float2 texcoord : TEXCOORD, float3 normal : NORMAL) : SV_TARGET
{
    Material matt;
    matt.diffuseColor = texture0.Sample(sampler0, texcoord);
    matt.normal = normalize(normal);
    matt.specExp = 1.4f;
    matt.specIntensity = 1.6f;

    float3 normalized = normalize(normal);
    //convert colour to linear space
    color = float4(color.rgb * color.rgb, color.a);

    //call helper function

    float4 finalColor;

    finalColor = CalcAmbient(normalized, matt.diffuseColor);
    finalColor += CalcDirectional(position.xyz, matt);
    
    return finalColor * texture0.Sample(sampler0, texcoord);
}

