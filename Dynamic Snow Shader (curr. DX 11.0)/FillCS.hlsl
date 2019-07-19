
#include "Inline.hlsl"
Texture2D UAV : register(t0);

cbuffer CB
{
    float FillRate;
    float3 padding;
};


//This is the incoming deformation texture
RWTexture2D<uint> source;


//called by dispatch(1, 1024, 1);
[numthreads(1,1024,1)]
void FillShader(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    //setting up the data conversion from uint to r16g16 formatted data 
    float2 usable;
    
    //Convert the data    
    usable = D3DX_R16G16_FLOAT_to_FLOAT2(source[dispatchThreadId.xy]);
    
    //Takes the pixel of the texture and applies the fill rate
    if (usable.x <= 0)
    {
        usable.x += FillRate;
    }
    

    //Converting back to UINT
    source[dispatchThreadId.xy] = D3DX_FLOAT2_to_R16G16_FLOAT(usable);

}