cbuffer CB0
{
	matrix WVPMatrix; //64 bytes
    matrix WVMatrix; //64bytes
};

cbuffer Light
{
    matrix world;
    float4 EyePos; //16
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


TextureCube cube0;
SamplerState sampler0;

struct VOut
{
	float4 position : SV_POSITION;
    float4 color : COLOR;
	float3 texcoord : TEXCOORD;
    float3 normal : NORMAL;
};

//Helper functions
float4 CalcAmbient(float3 normal, float4 color)
{
    //convert from [-1|1] to [0|1]
    float up = normal.y * 0.5f + 0.5f;
    //calculate and apply ambient value
    float3 Ambient = (float3) AmbientDown + up * (float3) AmbientRange;
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


VOut ReflectVS(float4 position : POSITION, float2 texcoord : TEXCOORD, float3 normal : NORMAL)
{
	VOut output;

	float4 default_color = { 1.0f, 1.0f, 1.0f, 1.0f };
    output.color = default_color;
    output.position = mul(WVPMatrix, position);
    //position relative to camera
    float3 wvpos = mul((float3x3) WVMatrix, (float3) position);
    //surface normal relative to camera
    float3 wvnormal = (float3) mul(WVMatrix, float4(normal, 1.0f));
    wvnormal = normalize(wvnormal);
    //obtain the reverse eyevector
    float3 eyer = -normalize(wvpos);
    //compute the reflection vector
    output.texcoord = 2.0 * dot(eyer, wvnormal) * wvnormal - eyer;
    output.normal = normal.xyz;
	return output;
}

float4 ReflectPS(float4 position : SV_POSITION, float4 color : COLOR, float3 texcoord : TEXCOORD, float3 normal: NORMAL) : SV_TARGET
{
    Material matt;
    matt.diffuseColor = cube0.Sample(sampler0, texcoord);
    matt.normal = normalize(normal);
    matt.specExp = 1.0f;
    matt.specIntensity = .9f;

    float3 normalized = normalize(normal);
    //convert colour to linear space
    color = float4(color.rgb * color.rgb, color.a);

    //call helper function

    float4 finalColor;

    finalColor = CalcAmbient(normalized, matt.diffuseColor);
    finalColor += CalcDirectional(position.xyz, matt);

    return finalColor* cube0.Sample(sampler0, texcoord);
}