#include "Inline.hlsl"

Texture2D UAV : register(t0);

RWTexture2D<uint> source;

[numthreads(32,32,1)]
void Init(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    source[dispatchThreadId.xy] = 0xffffffff;
}