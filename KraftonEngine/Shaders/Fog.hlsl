#include "Common/ConstantBuffers.hlsl"

#pragma pack_matrix(row_major)

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
    float4 worldPosH = mul(clipPos, InvViewProj);
    float3 worldPos = worldPosH.xyz / worldPosH.w;
    
    float3 cameraPos = CameraPos.xyz;
    float3 rayVec = worldPos - cameraPos;
    float rayLength = length(rayVec);
    float3 rayDir = rayVec / max(rayLength, 0.0001f);

    if (rayLength < 0.1f || depth < 0.00001f)
    {
        return float4(0, 0, 0, 0);
    }

    // Skybox (depth >= 1.0f) 일 때만 FogCutoffDistance를 적용하고, 일반 메쉬는 실제 거리(rayLength)를 사용합니다.
    float currentRayLength = (depth >= 1.0f) ? FogCutoffDistance : rayLength;
    float effectiveRayLength = max(0.0f, currentRayLength - StartDistance);
    
    if (effectiveRayLength <= 0.0f)
    {
        return float4(0, 0, 0, 0); 
    }

    // 3. Unreal Engine 방식의 완전한 지수 높이 안개 적분
    float falloff = max(0.00001f, FogHeightFalloff * 0.001f); // 언리얼 스케일 보정
    float density = FogDensity * 0.001f; // 언리얼 스케일 보정 (1000으로 나눔)

    // 안개 시작점의 높이 (StartDistance 고려)
    float3 startPos = cameraPos + rayDir * StartDistance;
    float h_s = startPos.z - FogBaseHeight;
    
    // 시작점에서의 안개 밀도
    float RayOriginTerms = density * exp2(clamp(-falloff * h_s, -127.0f, 127.0f));

    // Ray 방향에 따른 전체 높이 변화율 (정확한 적분을 위해 거리를 곱함)
    float FalloffTerm = falloff * rayDir.z * effectiveRayLength;
    
    // 언리얼 엔진 CalculateLineIntegralShared 와 동일한 로직
    // exp2를 사용하므로 FalloffTerm이 0일 때의 극한값은 ln(2) = 0.693147입니다.
    float LineIntegral = 0.693147f; 
    if (abs(FalloffTerm) > 0.0001f)
    {
        LineIntegral = (1.0f - exp2(clamp(-FalloffTerm, -127.0f, 127.0f))) / FalloffTerm;
    }

    float fogFactor = max(0.0f, RayOriginTerms * LineIntegral * effectiveRayLength);

    // 4. 최종 투명도
    float opacity = 1.0f - exp2(-fogFactor);
    opacity = clamp(opacity, 0.0f, FogMaxOpacity);

    return float4(FogColor.rgb, opacity);
}
