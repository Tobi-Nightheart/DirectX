#include "Inline.hlsl"

//Texture uses a resolution of 4 cm per pixel equalling to 40.96 m²
//This induces that each vWorldPosition has to be devided by 0.04 to give the appropriate pixel in the UAV
cbuffer Values : register(b0)
{
    float3 vWorldPos;
    float scale;
};

RWTexture2D<uint> HeightMap : register(u0);


float2 Modulus(float2 WorldPos, float2 TexSize)
{
    return WorldPos - (TexSize * floor(WorldPos / TexSize));
}

half CalcDeformation(float3 PointH, uint3 DTID)
{
    float dist = sqrt(pow(DTID.x - PointH.x, 2) + pow(DTID.y - PointH.z, 2));
    return (half) (PointH.y + pow(dist, 2) * scale);
}

[numthreads(32, 32, 1)]
void Deformation(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    //First the texture values need to be interpreted correctly and therefore converted
    half DeformationHeight;
    half FootHeight;
    float2 data;
    
    //scaled World position to read/write the correct data and offset the position to write in an 32x32 grid
    float3 sWorldPos = vWorldPos / 0.04;
    sWorldPos.x -= 16;
    sWorldPos.z -= 16;

    data = D3DX_R16G16_FLOAT_to_FLOAT2(HeightMap[sWorldPos.xz + DispatchThreadID.xy]);

    FootHeight = (half) vWorldPos.y;

         
	//affected_pixels= calulate_deformation(dynamic_object)
    //First lets try simple deformation TEST
    DeformationHeight = CalcDeformation(vWorldPos, DispatchThreadID);

    data.x = DeformationHeight;
    data.y = FootHeight;

    //Convert the data back into a uint
    uint result = D3DX_FLOAT2_to_R16G16_FLOAT(data.xy);

    //write the atomic minimum into the texture
    InterlockedMin(HeightMap[sWorldPos.xz + DispatchThreadID.xy], result, HeightMap[sWorldPos.xz + DispatchThreadID.xy]);
    
}