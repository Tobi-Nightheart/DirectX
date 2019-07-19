//Constant Buffers
cbuffer HeightConstants : register(b0)
{
    float4x4 ToHeight : packoffset(c0);

}

//Vertex Shader that outputs the depths
float4 HeightMapVS(float4 Pos : POSITION) : SV_POSITION
{
    return mul(Pos, ToHeight);
}