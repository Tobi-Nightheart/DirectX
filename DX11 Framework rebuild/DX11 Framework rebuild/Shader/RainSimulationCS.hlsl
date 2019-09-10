Texture2D<float4> NoiseTex : register(t0);
Texture2D<float> HeightTex : register(t1);

struct RainDrop
{
    float3 Pos;
    float3 Vel;
    float State;
};

RWStructuredBuffer<RainDrop> RainData : register(u0);

cbuffer RainCB : register(b0)
{
    float4x4 HeightSpace : packoffset(c0);
    float3 BoundCenter : packoffset(c4);
    float DT : packoffset(c4.w);
    float3 BoundHalfSize : packoffset(c5);
    float WindVariation : packoffset(c5.w);
    float2 WindForce : packoffset(c6);
    float VertSpeed : packoffset(c6.z);
    float HeightMapSize : packoffset(c6.w);
}

static const int g_iNumThreads = 32;
static const int g_iNumDispatch = 4;
static const int g_iDimSize = g_iNumDispatch * g_iNumThreads;

[numthreads(g_iNumThreads, g_iNumThreads, 1)]
void SimulateRain(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint GroupIdx = dispatchThreadId.x + dispatchThreadId.y * g_iDimSize;
    RainDrop cur = RainData[GroupIdx];
    
    cur.Pos += cur.Vel * DT;

    float2 offsetAmount = (cur.Pos.xz - BoundCenter.xz) / BoundHalfSize.xz;
    cur.Pos.xz -= BoundHalfSize.xz * ceil(0.5f * offsetAmount - 0.5f);

    if(abs(cur.Pos.y - BoundCenter.y) > BoundHalfSize.y)
    {
        float4 vNoiseNorm0 = NoiseTex.Load(int3(dispatchThreadId.xy, 0));
        float4 vNoise0 = (vNoiseNorm0 * 2.0f) - 1.0f;

        cur.Pos.xz = BoundCenter.xz + BoundHalfSize.xz * vNoise0.xy;

        cur.Pos.y = BoundCenter.y + BoundHalfSize.y;
        cur.Pos.y -= dot(vNoiseNorm0.zw, 0.2f) * BoundHalfSize.y;

        cur.Vel.xz = lerp(WindForce, WindForce * vNoise0.zw, WindVariation);
        cur.Vel.y = VertSpeed;
    }

    float4 posInHeight = float4(cur.Pos, 1.0f);
    posInHeight = mul(posInHeight, HeightSpace);
    posInHeight.xy = 0.5f * (posInHeight.xy + 1.0f);
    posInHeight.y = 1.0f - posInHeight.y;
    posInHeight.xy *= HeightMapSize;

    float height = HeightTex.Load(uint3(posInHeight.xy, 0));
   
    cur.State = posInHeight.z < height ? 1.0f : -1.0f;

    RainData[GroupIdx] = cur;
}