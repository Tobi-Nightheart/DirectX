#include "Inline.hlsl"

Texture2D UAV : register(t0);

RWTexture2D<uint> source;

[numthreads(1,1024,1)]
void Init(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    float2 usable;

    usable = D3DX_R16G16_FLOAT_to_FLOAT2(source[dispatchThreadId.xy]);

    usable.y = 15.0f;

    source[dispatchThreadId.xy] = D3DX_FLOAT2_to_R16G16_FLOAT(usable);
}