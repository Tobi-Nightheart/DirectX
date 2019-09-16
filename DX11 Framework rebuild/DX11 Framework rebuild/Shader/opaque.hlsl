cbuffer OpaqueCB : register(b0)
{
    float4x4 WVP : packoffset(c0);
    float4x4 World : packoffset(c4);
}


float4 OpaqueVS(float3 Pos : POSITION) : SV_Position
{
    return mul(WVP, float4(Pos, 1.0f));
}