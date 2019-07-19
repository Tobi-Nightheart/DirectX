#include "Inline.hlsl"
//ref DetailTessellation11 in sample sdk folder G:
//consider density based addition for tessellation

//Textures
Texture2D g_baseTexture : register(t0);
Texture2D g_deformTexture : register(t1);
Texture2D HeightMap : register(t2);


//Samplers
SamplerState g_samLinear : register(s0);

//Constant buffers
cbuffer cbMain : register(b0)
{
    matrix mWorld;
    matrix mWVP;
    float scale;
    float3 padding;
}

cbuffer cbMaterial : register(b1)
{
    float4 MatAmbC; //mat ambient colour
    float4 MatDifC; //mat diffuse colour

    float4 vLightPos; //light pos in WS

    float4 vCamPos; //cam location
    float4 fBaseTextureRepeat; //the tiling factor for base and normal textures
}

//Structures
struct VertexIn
{
    float3 inPos : POSITION;
    float2 inTexCoord : TEXCOORD;
    float3 vInNormal : NORMAL;
};

//Input control point
struct VertexOut
{

    float3 vPos : POSITION0;
    float2 texCoord : TEXCOORD;
    float3 vNormal : NORMAL;
    float3 vSnowPos : POSITION1;
    bool swap : OUTPUT;
};

//Output patch constant data
struct PatchTess
{
    float Edges[3] : SV_TessFactor;
    float Inside : SV_InsideTessFactor;
    
};

struct HullOut
{
    float3 vPos : POSITION0;
    float3 vNormal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 vSnowPos : POSITION1;
    
    bool swap : OUTPUT;
};

//Domain shader output
struct DomainOut
{
    float2 texCoord : TEXCOORD;
    float3 vNormal : NORMAL;
    float4 vPosition : SV_POSITION0;
    float3 vSnowPos : POSITION1;
    
    bool swap : OUTPUT;
};


//Helper Functions
float4 ComputeIllumination(float2 texCoord, float3 vLightTS, float3 vViewTS)
{
    //Add light calculation
    float4 cFinalColour = float4(1.0f, 1.0f, 1.0f, 1.0f);
    return cFinalColour;
}

//Calculating the exact elevation and deformation values
float CalcElevation(float DeformH, float SnowH, float FootH)
{
    //Depression Variables
    float FootDist;
    float DeprDist;
    //Elevation Variables
    float MaxElevDist, ElevHeight, ElevRatio, ElevDist;

    //Calculate the distance of depression
    DeprDist = sqrt(SnowH - FootH);

    //Calculate the distance from the foot
    FootDist = sqrt(DeformH - FootH);
    
    ElevDist = FootDist - DeprDist;

    //Calculating the maximum distance of the elevation (in perspective to the depression)
    MaxElevDist = scale * (FootDist - DeprDist);
    
    //Calculate the ratio of elevation
    ElevRatio = ElevDist / MaxElevDist;
    
    //Calculate the height of the elevation at this point
    ElevHeight = MaxElevDist * scale;

    return (pow(0.5 - 2 * ElevRatio, 2) + 1) * ElevHeight;

}
//Draw Shader VERTEX
VertexOut SnowVS(VertexIn input)
{
    VertexOut Out;

    //Pass snow height for later use in PS
    Out.vSnowPos = input.inPos;
    bool swap = false;

    //Pass through the new position along
    Out.vPos = input.inPos;

    //Propagate texture coordinate
    Out.texCoord = input.inTexCoord * fBaseTextureRepeat.x;

    //pass normal
    Out.vNormal = input.vInNormal;

    //assign the texture
    Out.swap = swap;

    return Out;
}

//Tessellation stage
//Patch Constant Function
PatchTess ConstantsHS(InputPatch<VertexOut, 3> input, uint PatchID : SV_PrimitiveID)
{
    PatchTess Out = (PatchTess)0;
    
    //Find center of patch in WS
    float3 centerL = 0.25f * (input[0].vPos + input[1].vPos + input[2].vPos);
    float3 centerW = mul( mWorld, float4(centerL, 1.0f)).xyz;

    float d = distance(centerW, (float3) vCamPos);

    //Tessellate the patch based on distance from the eye such that the tessellation 
    //is 0 if d >= d0 and 20 if d <= d1. The interval [d0, d1] defines the range we tessellate in.
    const float d0 = 20.0f;
    const float d1 = 100.0f;
    
    float tess = 20.0f * saturate((d1 - d) / (d1 - d0));

    //Uniformly tessellate patch
    Out.Edges[0] = tess;
    Out.Edges[1] = tess;
    Out.Edges[2] = tess;

    Out.Inside = tess;

    return Out;
}

//hull shader
[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantsHS")]
[maxtessfactor(20.0f)]
HullOut SnowHS(InputPatch<VertexOut, 3> inputPatch, uint uCPID : SV_OutputControlPointID, uint patchID : SV_PrimitiveID)
{
    HullOut Output;

    //Copy inputs to outputs
    Output.vPos = inputPatch[uCPID].vPos.xyz;
    Output.vNormal = inputPatch[uCPID].vNormal;
    Output.texCoord = inputPatch[uCPID].texCoord;
    
    //Pass snow height for later use in PS
    Output.vSnowPos = inputPatch[uCPID].vSnowPos;

    //pass the texture swap variable
    //Output.swap = inputPatch[uCPID].swap; DEBUG why is this line breaking the code
    Output.swap = inputPatch[0].swap;

    return Output;
}


//domain shader
[domain("tri")]
DomainOut DS(PatchTess patchTess, float3 uv : SV_DomainLocation, const OutputPatch<HullOut, 3> Tri)
{
    DomainOut Output;

    //Interpolate Position with barycentric coordinates
    float3 vWorldPos = uv.x * Tri[0].vPos + uv.y * Tri[1].vPos + uv.z * Tri[2].vPos;

    //Interpolate Normal with barycentric coordinates and normalize
    float3 vNormal = uv.x * Tri[0].vNormal + uv.y * Tri[1].vNormal+ uv.z * Tri[2].vNormal;
    vNormal = normalize(vNormal);

    //Interpolate Texture Coordinates with barycentric coordinates
    Output.texCoord = uv.x * Tri[0].texCoord + uv.y * Tri[1].texCoord + uv.z * Tri[2].texCoord;
    
    //Pass snow height for later use in PS
    Output.vSnowPos = Tri[0].vSnowPos;
    
    //pass the texture variable
    Output.swap = Tri[0].swap;
    
    //scale the position to texture scale (4cm =1 px)
    float3 sWorld = vWorldPos.xyz / 0.04;
    //Sample the Heightmap
    float2 data = (float2) HeightMap.Load(sWorld);
    float deformationHeight = data.x;
    float footHeight = data.y;
    if (footHeight < vWorldPos.y)
    {

        vWorldPos.y = min(Output.vSnowPos.y, deformationHeight);
        Output.swap = true;
    }



    //Assign values to output and transform position in WVP space
    Output.vPosition = mul(mWVP, float4(vWorldPos, 1.0f));
    Output.vNormal = vNormal;

    

    

    return Output;
}

//Drawing Shader PIXEL not sure about this
//Pixel Shader

    //snow_height = pixel_input.snow_height
    //deformation_height = sample_deformation_heihgtmap()
    //calculate_deformed_normal()

float4 SnowPS(DomainOut input) : SV_TARGET
{

    //input.vSnowPos;

    float4 cResultColour = float4(0, 0, 0, 1);

    //compute resulting colour for pixel
    //cResultColour = ComputeIllumination(input.texCoord, vLightTS, vViewTS) * g_baseTexture.Sample(g_samLinear, input.texCoord.xy);
   
    if (input.swap)
    {
       cResultColour = g_deformTexture.Sample(g_samLinear, input.texCoord.xy);
        return cResultColour;
    }
    cResultColour = g_baseTexture.Sample(g_samLinear, input.texCoord.xy);

    return cResultColour;
}