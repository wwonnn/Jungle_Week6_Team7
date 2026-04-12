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
    float4 worldPos = mul(InvViewProj, clipPos);
    worldPos.xyz /= worldPos.w;
    
    float3 cameraPos = CameraPos.xyz;
    float3 rayVec = worldPos.xyz - cameraPos;
    float rayLength = length(rayVec);
    float3 rayDir = rayVec / max(rayLength, 0.0001f);

    if (rayLength < 0.1f || depth < 0.00001f)
    {
        return float4(0, 0, 0, 0);
    }

    float currentRayLength = (depth >= 1.0f) ? FogCutoffDistance : rayLength;
    float effectiveRayLength = max(0.0f, currentRayLength - StartDistance);
    
    if (effectiveRayLength <= 0.0f)
    {
        return float4(0, 0, 0, 0); 
    }

    // 3. Unreal Engine 방식의 완전한 지수 높이 안개 적분
    float falloff = max(0.0001f, FogHeightFalloff); 
    float density = FogDensity; 

    float h_c = cameraPos.z - FogBaseHeight;
    // 대상점의 높이를 계산 (StartDistance를 고려한 실제 안개 시작점부터의 오프셋)
    float3 startPos = cameraPos + rayDir * StartDistance;
    float3 endPos = cameraPos + rayDir * currentRayLength;
    
    float h_s = startPos.z - FogBaseHeight;
    float h_e = endPos.z - FogBaseHeight;
    
    float fogFactor = 0.0f;
    
    // 수직 변화량
    float slope = rayDir.z;

    if (abs(slope) > 0.001f)
    {
        // 적분 공식: (Density/Falloff) * (exp(-Falloff * h_start) - exp(-Falloff * h_end)) / slope
        // float의 지수 연산 한계(exp(88)까지 가능)를 고려하여 clamp 범위를 넓힙니다.
        float exp_s = exp(clamp(-falloff * h_s, -80.0f, 80.0f));
        float exp_e = exp(clamp(-falloff * h_e, -80.0f, 80.0f));
        
        fogFactor = (density / (falloff * slope)) * (exp_s - exp_e);
    }
    else
    {
        // 수평인 경우: 거리에 따른 단순 감쇠
        fogFactor = density * exp(clamp(-falloff * h_s, -80.0f, 80.0f)) * effectiveRayLength;
    }

    // 4. 최종 투명도
    float opacity = 1.0f - exp(-max(0.0f, fogFactor));
    opacity = clamp(opacity, 0.0f, FogMaxOpacity);

    return float4(FogColor.rgb, opacity);
}
