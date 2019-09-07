cbuffer Model : register(b0)
{
    matrix mWVP : packoffset(c0);
    matrix mWorld : packoffset(c4);
};

cbuffer Light : register(b1)
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

Texture2D texture0 : register(t1);
SamplerState sampler0 : register(s0);

struct vIn
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
};

struct vOut
{
    float3 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
};

struct Material
{
    float3 normal;
    float4 cDiffuse;
    float specExp;
    float specIntensity;
};

float3 CalcAmbient(float3 normal, float3 color)
{
    //converting from [-1,1] to [0,1]
    float up = normal.y * 0.5f + 0.5f;
    //calculating hemispheric ambient term
    float3 Ambient = vAmbientDown + up * vAmbientRange;
    return Ambient * color;
}

float3 CalcDirectional(float3 pos, Material mat)
{
    //Phong Diffuse
    float NDotL = dot(vDirToLight, mat.normal);
    float3 finalColor = cDirLight.rgb * saturate(NDotL);

    //Blinn Diffuse
    float3 ToEye = eyePos - pos;
    ToEye = normalize(ToEye);
    float3 HalfWay = normalize(ToEye + vDirToLight);
    float NDotH = saturate(dot(HalfWay, mat.normal));
    finalColor += cDirLight.rgb * pow(NDotH, mat.specExp) * mat.specIntensity;

    return finalColor * mat.cDiffuse.rgb;
}


vOut VShader(vIn input)
{
    vOut output;
    //mental note this might need to be changed
    output.position = mul((float3x3) mWVP, input.position);
    output.texcoord = input.texcoord;
    output.normal = mul((float3x3) mWorld, input.normal);

    return output;
}

float4 PShader(vOut input)
{
    float3 normal = normalize(input.normal);
    
    Material mat;
    mat.cDiffuse = float4(0.9f, 0.9f, 0.9f, 1.0f);
    mat.normal = normal;
    mat.specExp = 1.4f;
    mat.specIntensity = 1.6f;

    float4 color = texture0.Sample(sampler0, input.texcoord);
    //conversion to linear space
    color = color.rgb * color.rgb, color.a;

    float4 finalColor = float4(CalcAmbient(normal, (float3) color), 1.0f);

    finalColor.rgb += CalcDirectional(input.position, mat);

    return finalColor;
}