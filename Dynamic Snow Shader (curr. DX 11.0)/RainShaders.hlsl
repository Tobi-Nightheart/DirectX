//vertex shader output struct
struct VSRainOutput
{
    float4 Pos : SV_POSITION;
    float Clip : SV_CLIPDISTANCE0;
    float2 Tex : TEXCOORD0;
};

struct PSIn
{
    float4 Pos : SV_POSITION;
    float Clip : SV_CLIPDISTANCE0;
    float2 Tex : TEXCOORD0;
};

cbuffer DrawConstants : register(b0)
{
    float4x4 ViewProj : packoffset(c0);
    float3 ViewDir : packoffset(c4);
    float Scale : packoffset(c4.w);
    float4 AmbientColor : packoffset(c5);
}

//constant values to expand raindrop to triangle
static const float2 arrBasePos[6] =
{
    float2(1.0f, -1.0f),
    float2(1.0f, 0.0f),
    float2(-1.0f, -1.0f),

    float2(-1.0f, -1.0f),
    float2(1.0f, 0.0f),
    float2(-1.0f, 0.0f),
};

//constant values for UV
static const float2 arrUV[6] =
{
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f),
    float2(0.0f, 0.0f),

    float2(0.0f, 0.0f),
    float2(1.0f, 1.0f),
    float2(0.0f, 1.0f),
};

//struct for the simultation data used by the VS
struct CSBuffer
{
    float3 Pos;
    float3 Vel;
    float State;
};

//Simulation data used by VS
StructuredBuffer<CSBuffer> RainData : register(t0);

//Vertex Shader
VSRainOutput VS_Rain(uint VertexID : SV_VERTEXID)
{
    VSRainOutput output;

    //Get the current rain drop
    CSBuffer curDrop = RainData[VertexID / 6];

    //get the base position
    float3 pos = curDrop.Pos;

    //Find the expension directions
    float3 rainDir = normalize(curDrop.Vel);
    float3 rainRight = normalize(cross(ViewDir, rainDir));

    //Extend the drop position to the streak corners
    float2 offsets = arrBasePos[VertexID % 6];
    pos += rainRight * offsets.x * Scale * 0.025f;
    pos += rainDir * offsets.y * Scale;

    //Transform each corner to projected space
    output.Pos = mul(float4(pos, 1.0f), ViewProj);

    //Just Copy the UV Coordinates
    output.Tex = arrUV[VertexID % 6];

    //Clip particles that collided with the ground
    output.Clip = curDrop.State;

    return output;
}

//Pixel shader with pixel shader specific variables
Texture2D RainStreakTex : register(t0);
SamplerState LinearSampler : register(s0);

float4 PS_Rain(PSIn input) : SV_TARGET
{
    float fTexAlpha = RainStreakTex.Sample(LinearSampler, input.Tex).r;
    return float4(AmbientColor.rgb, AmbientColor.a * fTexAlpha);
}


