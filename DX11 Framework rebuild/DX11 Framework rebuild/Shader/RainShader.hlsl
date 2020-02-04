//vertex shader output struct
struct VSRainOutput
{
    float4 Pos : SV_POSITION;
    float Clip : SV_CLIPDISTANCE;
    float Tex : TEXCOORD0;
};

cbuffer DrawConstants : register(b0)
{
    float4x4 ViewProj : packoffset(c0);
    float3 ViewDir : packoffset(c4);
    float Scale : packoffset(c4.w);
    float4 AmbientColor : packoffset(c5);
}

//constant values to expand raindrop quad
static const float2 arrBasePos[6] =
{
    float2(1.0f, -1.0f),
    float2(1.0f, 0.0f),
    float2(-1.0f, -1.0f),

    float2(-1.0f, -1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 0.0f),
};

//constant values for UV
static const float2 arrUV[6] =
{
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f),
    float2(0.0f, 1.0f),
    
    float2(0.0f, 0.0f),
    float2(1.0f, 1.0f),
    float2(0.0f, 1.0f),
};

//struct for the simulation used by the VS
struct CSBuffer
{
    float3 Pos;
    float3 Vel;
    float State;
};

//Simulation data used by VS
StructuredBuffer<CSBuffer> RainData : register(t0);

//Vertex Shader
VSRainOutput VSRain(uint VertexID : SV_VertexID)
{
    VSRainOutput output;
    
    //get current raindrop
    CSBuffer drop = RainData[VertexID / 6];
    
    //get the base position
    float3 pos = drop.Pos;
    
    //find the expension directions
    float3 rainDir = normalize(drop.Vel);
    float3 rainRight = normalize(cross(ViewDir, rainDir));
    
    //extend the drop position to the streak corners
    float2 offsets = arrBasePos[VertexID % 6];
    pos += rainRight * offsets.x * Scale * 0.025f;
    pos += rainDir * offsets.y * Scale;
    
    //transform each corner in projected space
    output.Pos = mul(float4(pos, 1.0f), ViewProj);
    
    //copy UV coords
    output.Tex = arrUV[VertexID % 6];
    
    //clip particles that collided with the ground
    output.Clip = drop.State;
    
    return output;
}

//Pixel shader variables
Texture2D RainStreakTex : register(t1);
SamplerState LinearSampler : register(s0);

float4 PSRain(VSRainOutput input) : SV_TARGET
{
    float fTexAlpha = RainStreakTex.Sample(LinearSampler, input.Tex).r;
    return float4(AmbientColor.rgb, AmbientColor.a * fTexAlpha);
}