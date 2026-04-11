#include "Common/ConstantBuffers.hlsl"

cbuffer FogBuffer : register(b5)
{
    float4 FogColor;

    float FogDensity;
    float FogHeightFalloff;
    float StartDistance;
    float FogCutoffDistance;
    
    float FogMaxOpacity;
    float FogBaseHeight;
    float2 _Padding0;

    float4x4 InvViewProj;
    float4 CameraPos;
};

// t0: SceneDepth (0: Near, 1: Far)
Texture2D<float> SceneDepthTex : register(t0);
SamplerState LinearSampler : register(s0);

struct PS_Input
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

PS_Input VS(uint vertexID : SV_VertexID)
{
    PS_Input output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return output;
}

float4 PS(PS_Input input) : SV_TARGET
{
    int3 texCoord = int3(input.position.xy, 0);
    float depth = SceneDepthTex.Load(texCoord);

    // 1. 월드 포지션 복구
    float2 ndc_xy = input.uv * 2.0f - 1.0f;
    ndc_xy.y = -ndc_xy.y;
    
    float4 clipPos = float4(ndc_xy, depth, 1.0f);
    float4 worldPos = mul(clipPos, InvViewProj);
    worldPos.xyz /= worldPos.w;
    
    float3 cameraPos = CameraPos.xyz;
    float3 rayVec = worldPos.xyz - cameraPos;
    float rayLength = length(rayVec);
    float3 rayDir = rayVec / max(rayLength, 0.0001f);

    // 2. 안개 적용 거리 계산
    // 아주 가까운 거리(예: 10 units) 이하이거나 depth가 0에 가까우면 안개 생략
    if (rayLength < 10.0f || depth < 0.00001f)
    {
        return float4(0, 0, 0, 0);
    }

    float currentRayLength = (depth >= 1.0f) ? FogCutoffDistance : rayLength;
    float effectiveRayLength = max(0.0f, currentRayLength - StartDistance);
    
    if (effectiveRayLength <= 0.0f)
    {
        return float4(0, 0, 0, 0); 
    }

    // 3. Exponential Height Fog 공식 (UE 방식 - BaseHeight 적용)
    float falloff = max(0.0001f, FogHeightFalloff * 0.01f); 
    float density = FogDensity * 0.1f;
    
    // 포그 액터의 Z 높이를 기준으로 한 카메라의 상대적 높이
    float cameraHeight = cameraPos.z - FogBaseHeight;
    float rayZ = rayDir.z * effectiveRayLength;
    
    float fogFactor = 0.0f;
    // 지수값 제한으로 NaN/Inf 방지
    float exponent = clamp(falloff * rayZ, -20.0f, 20.0f);
    float heightExp = clamp(-falloff * cameraHeight, -20.0f, 20.0f);
    
    if (abs(rayZ) > 0.001f)
    {
        fogFactor = (density / falloff) * exp(heightExp) * (1.0f - exp(-exponent)) / rayDir.z;
    }
    else
    {
        fogFactor = density * exp(heightExp) * effectiveRayLength;
    }

    // 4. 최종 투명도 및 블렌딩
    float opacity = 1.0f - exp(-max(0.0f, fogFactor));
    opacity = clamp(opacity, 0.0f, FogMaxOpacity);

    return float4(FogColor.rgb, opacity);
}
