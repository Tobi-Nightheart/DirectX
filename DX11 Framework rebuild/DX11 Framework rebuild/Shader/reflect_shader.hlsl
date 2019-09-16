cbuffer CB0 : register(b0)
{
    matrix WVP : packoffset(c0);
    matrix WV : packoffset(c4);
};

cbuffer LightCB : register(b1)
{
    float3 eyePos : packoffset(c0);
    float pack1 : packoffset(c0.w);
    float3 vDirToLight : packoffset(c1);
    float pack2 : packoffset(c1.w);
    float4 cDirLight : packoffset(c2);
    float3 vAmbientDown : packoffset(c3);
    float pack3 : packoffset(c3.w);
    float3 vAmbientRange : packoffset(c4);
    float pack4 : packoffset(c4.w);
};

TextureCube cube0 : register(t0);
SamplerState sampler0 : register(s0);

struct Material
{
    float3 normal;
    float4 cDiffuse;
    float specExp;
    float specIntensity;
};

struct VOut
{
    float3 position : SV_POSITION;
    float3 texcoord : TEXCOORD;
    float3 normal : NORMAL;
};

float4 CalcAmbient(float3 normal, float3 color)
{
    float up = normal.y * 0.5f + 0.5f;
    float3 Ambient = vAmbientDown + up * vAmbientRange;
    return Ambient * color;
}

float3 CalcDirectional(float3 pos, Material mat)
{
    //Phong diffuse
    float NDotL = dot(vDirToLight, mat.normal);
    float3 finalColor = cDirLight.rgb * saturate(NDotL);
    //Blinn Diffuse
    float3 ToEye = eyePos - pos;
    ToEye = normalize(ToEye);
    float3 HalfWay = saturate(dot(HalfWay, mat.normal));
    finalColor += cDirLight.rgb * pow(NDotL, mat.specExp) * mat.specIntensity;

    return float4(finalColor * mat.cDiffuse.rgb, 1.0f);
}

VOut ReflectVS(float3 position : POSITION, float2 texcoord : TEXCOORD, float3 normal : NORMAL)
{
    VOut output;
    output.position = (float3) mul(WVP, float4(position, 1.0f));
    //calculate pos in WorldView-Space
    float3 wvPos = (float3) mul(WV, float4(position, 1.0f));
    //surface normal in WorldView-Space
    float3 wvNormal = (float3) mul(WV, float4(normal, 1.0f));
    wvNormal = normalize(wvNormal);
    //obtain the reverse eyeVector
    float3 eyeR = -normalize(wvPos);
    //compute the reflection vector
    output.texcoord = 2.0f * dot(eyeR, wvNormal) * wvNormal - eyeR;
    output.normal = normal;
    return output;
}

float4 ReflectPS(float3 position : SV_POSITION, float3 texcoord : TEXCOORD, float3 normal : NORMAL)
{
    Material matt;
    matt.cDiffuse = cube0.Sample(sampler0, texcoord);
    //conversion to  linear space
    matt.cDiffuse = matt.cDiffuse.rgb * matt.cDiffuse.rgb, matt.cDiffuse.a;

    matt.normal = normalize(normal);
    matt.specExp = 1.0f;
    matt.specIntensity = 0.9f;

    float3 normalized = normalize(normal);
    
    float4 finalColor = CalcAmbient(normal, matt.cDiffuse.rgb);
    finalColor += CalcDirectional(position, matt);
    
    return finalColor;
}

