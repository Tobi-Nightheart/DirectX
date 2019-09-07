//ref DetailTessellation11 in sample sdk folder G:
//consider density based addition for tessellation

//Textures
Texture2D g_baseTexture : register(t0);
Texture2D g_nmhTexture : register(t1);
Texture2D HeightMap : register(t2);


//Samplers
SamplerState g_samLinear : register(s0);

//Constant buffers
cbuffer cbMain : register(b0)
{
    matrix mWorld;
    matrix mVP;
    float4 vTessellationFactor;
    float4 vDetailTessellationHeightScale;
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
struct VS_INPUT
{
    float3 inPos : POSITION;
    float2 inTexCoord : TEXCOORD;
    float3 vInNormal : NORMAL;
    float3 vInBinormal : BINORMAL;
    float3 vInTanget : TANGENT;
};


//TESSELATION STAGE

//Input control point
struct VS_OUTPUT_HS_INPUT
{

    float3 vPos : WORLDPOS;
    float3 vNormal : NORMAL;
    float fVertexDistanceFactor : VERTEXDISTANCEFACTOR;

    float2 texCoord : TEXCOORD0;
    float3 vLightTS : TANGENT;

  //  float snowHeight;
};

//Output patch constant data
struct HS_CONSTANT_DATA_OUTPUT
{
    float Edges[3] : SV_TessFactor;
    float Inside : SV_InsideTessFactor;
    
};

struct HS_CONTROL_POINT_OUTPUT
{
    float3 vWorldPos : WORLDPOS;
    float3 vNormal : NORMAL;
    float2 texCoord : TEXCOORD0;
    float3 vLightTS : TANGENT;
};

//Domain shader output
struct DS_OUTPUT
{
    float2 texCoord : TEXCOORD0;
    float3 vLightTS : TANGENT;
    float4 vPosition : SV_POSITION;
};


struct PS_INPUT
{
    float2 texCoord : TEXCOORD0;
    float3 vLightTS : TANGENT;
};


//Helper Functions
float4 ComputeIllumination(float2 texCoord, float3 vLightTS, float3 vViewTS)
{
    //Sample the normal from normal map for the given texture sample:
    float3 vNormalTS = normalize((float3) g_nmhTexture.Sample(g_samLinear, texCoord) * 2.0f - 1.0f);

    //Sample base map
    float4 cBaseColour = g_baseTexture.Sample(g_samLinear, texCoord);

    //Compute diffuse colour component
    float4 cDiffuse = saturate(dot(vNormalTS, vLightTS)) * MatDifC;

    //Compute Specular if desired ADD HERE
    float4 cSpecular = 0;

    //Composite final colour
    float4 cFinalColour = (MatAmbC + cDiffuse) * cBaseColour + cSpecular;

    return cFinalColour;
}

//Draw Shader VERTEX

//ADDITIONAL Vertex Shader DATA

    //snow_height = vertex.Z
    //deformation_height = sample_deformation_heightmap()
    //vertex.Z = min(snow_heicht, defromation_heightmap)
    //pixel_input.snow_height = snow_height !!! pass this through the other shaders

VS_OUTPUT_HS_INPUT SnowVS(VS_INPUT input)
{
    VS_OUTPUT_HS_INPUT Out;

    //Algorithm to draw the deformation
    float snowHeight = input.inPos.z;
    //float deformationHeight = HeightMap.Sample(g_samLinear, input.inPos.xz).y; //find out how to find the pos in relation to model
    //input.inPos.z = min(snowHeight, deformationHeight);
    //Out.snowHeight = snowHeight;


    //Compute position in world space
    float3 vPositionWS = mul(input.inPos.xyz, (float3x3) mWorld);

    //Compute denormalized light vector in world space
    float3 vLightWS = vLightPos.xyz - vPositionWS.xyz;
    //need to invert Z for correct lighting
    vLightWS.z = -vLightWS.z;

    //Propagate texture coordinate
    Out.texCoord = input.inTexCoord * fBaseTextureRepeat.x;

    //Transform normal, tangent and binormal vectors from object space to homogenous projection space and normalize
    float3 vNormalWS = mul(input.vInNormal, (float3x3) mWorld);
    float3 vTangentWS = mul(input.vInTanget, (float3x3) mWorld);
    float3 vBinormalWS = mul(input.vInBinormal, (float3x3) mWorld);

    vNormalWS = normalize(vNormalWS);
    vTangentWS = normalize(vTangentWS);
    vBinormalWS = normalize(vBinormalWS);

    //output normal
    Out.vNormal = vNormalWS;

    //calculate tangent basis
    float3x3 mWorldToTangent = float3x3(vTangentWS, vBinormalWS, vNormalWS);

    //propagate light vector (in tangent space)
    Out.vLightTS = mul(mWorldToTangent, vLightWS);

    //Write world pos
    Out.vPos = float3(vPositionWS.xyz);

    //Min and max distance should be chosen according to scene quality requirments
    const float fMinDist = 20.0f;
    const float fMaxDist = 250.0f;

    //calculate distance between vertex and camera, and vertex distance factor issued from it
    float fDistance = distance(vPositionWS, (float3) vCamPos);
    Out.fVertexDistanceFactor = 1.0 - clamp(((fDistance - fMinDist) / (fMaxDist - fMinDist)), 0.0f, 1.0 - vTessellationFactor.z / vTessellationFactor.x);


    return Out;
}

//Tessellation stage

//Patch Constant Function
HS_CONSTANT_DATA_OUTPUT ConstantsHS(InputPatch<VS_OUTPUT_HS_INPUT, 3> input, uint PatchID : SV_PrimitiveID)
{
    HS_CONSTANT_DATA_OUTPUT Out = (HS_CONSTANT_DATA_OUTPUT)0;
    float4 vEdgeTessellationFactors = vTessellationFactor.xxxy;
    
    //Calculate edge scale factor from vertex scale factor: simply compute average tess factor between the two verticies making up an edge
    vEdgeTessellationFactors.x = 0.5 * (input[1].fVertexDistanceFactor + input[2].fVertexDistanceFactor);
    vEdgeTessellationFactors.y = 0.5 * (input[2].fVertexDistanceFactor + input[0].fVertexDistanceFactor);
    vEdgeTessellationFactors.z = 0.5 * (input[0].fVertexDistanceFactor + input[1].fVertexDistanceFactor);
    vEdgeTessellationFactors.w = vEdgeTessellationFactors.x;

    //Multiply them by global tessellation factor
    vEdgeTessellationFactors *= vTessellationFactor.xxxy;

    //assing tessellation levels
    Out.Edges[0] = vEdgeTessellationFactors.x;
    Out.Edges[1] = vEdgeTessellationFactors.y;
    Out.Edges[2] = vEdgeTessellationFactors.z;
    Out.Inside = vEdgeTessellationFactors.w;

    return Out;
}

//hull shader
[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantsHS")]
[maxtessfactor(15.0)]
HS_CONTROL_POINT_OUTPUT SnowHS(InputPatch<VS_OUTPUT_HS_INPUT, 3> inputPatch, uint uCPID : SV_OutputControlPointID)
{
    HS_CONTROL_POINT_OUTPUT Output = (HS_CONTROL_POINT_OUTPUT)0;

    //Copy inputs to outputs
    Output.vWorldPos = inputPatch[uCPID].vPos.xyz;
    Output.vNormal = inputPatch[uCPID].vNormal;
    Output.texCoord = inputPatch[uCPID].texCoord;
    Output.vLightTS = inputPatch[uCPID].vLightTS;

    return Output;
}


//domain shader
[domain("tri")]
DS_OUTPUT DS(HS_CONSTANT_DATA_OUTPUT input, float3 BarycentricCoordinates : SV_DomainLocation, const OutputPatch<HS_CONTROL_POINT_OUTPUT, 3> TrianglePatch)
{
    DS_OUTPUT Output = (DS_OUTPUT) 0;

    //Interpolate world space pos with barycentric coordinates
    float3 vWorldPos = BarycentricCoordinates.x * TrianglePatch[0].vWorldPos + BarycentricCoordinates.y * TrianglePatch[1].vWorldPos + BarycentricCoordinates.z * TrianglePatch[2].vWorldPos;

    //Interpolate world space normal and renormalize it
    float3 vNormal = BarycentricCoordinates.x * TrianglePatch[0].vNormal + BarycentricCoordinates.y * TrianglePatch[1].vNormal + BarycentricCoordinates.z * TrianglePatch[2].vNormal;
    vNormal = normalize(vNormal);

    //Interpolate other inputs with barycentric coordinates
    Output.texCoord = BarycentricCoordinates.x * TrianglePatch[0].texCoord + BarycentricCoordinates.y * TrianglePatch[1].texCoord + BarycentricCoordinates.z * TrianglePatch[2].texCoord;
    float3 vLightTS = BarycentricCoordinates.x * TrianglePatch[0].vLightTS + BarycentricCoordinates.y * TrianglePatch[1].vLightTS + BarycentricCoordinates.z * TrianglePatch[2].vLightTS;

    //calculate the mip level to fetch normal from
    float fHeightMapMIPLevel = clamp((distance(vWorldPos, (float3) vCamPos) - 100.0f) / 100.0f, 0.0f, 3.0f);

    //sample normal and height map
    float4 vNormalHeight = g_nmhTexture.SampleLevel(g_samLinear, Output.texCoord, fHeightMapMIPLevel);

    //Displace vertex along normal
    vWorldPos += vNormal * (vDetailTessellationHeightScale.x * (vNormalHeight.w - 1.0f));

    //Transform world pos with viewProjection matrix
    Output.vPosition = mul(float4(vWorldPos.xyz, 1.0f), mVP);

    //Per Pixel lighting
    Output.vLightTS = vLightTS;

    return Output;
}

//Drawing Shader PIXEL not sure about this
//Pixel Shader

    //snow_height = pixel_input.snow_height
    //deformation_height = sample_deformation_heihgtmap()
    //calculate_deformed_normal()

float4 SnowPS(PS_INPUT input) : SV_TARGET
{



    float4 cResultColour = float4(0, 0, 0, 1);
    float3 vViewTS = float3(0, 0, 0);

    //normalize tangent space light vector
    float3 vLightTS = normalize(input.vLightTS);

    //compute resulting colour for pixel
    cResultColour = ComputeIllumination(input.texCoord, vLightTS, vViewTS) * g_baseTexture.Sample(g_samLinear, input.texCoord.xy);

    return cResultColour;
}