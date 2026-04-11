#include "Common/ConstantBuffers.hlsl"

cbuffer FogBuffer : register(b5)
{
    float4 FogColor;

    float FogDensity;
    float FogHeightFalloff;
    float StartDistance;
    float FogCutoffDistance;
    
    float FogMaxOpacity;
    float3 _Padding0;

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

    // NDC 좌표 계산
    float2 ndc = input.uv * 2.0f - 1.0f;
    ndc.y = -ndc.y;
    
    // 월드 포지션 복구
    float4 clipPos = float4(ndc, depth, 1.0f);
    float4 worldPos = mul(clipPos, InvViewProj);
    worldPos /= worldPos.w;
    
    float3 cameraPos = CameraPos.xyz;
    float3 rayDir = worldPos.xyz - cameraPos;
    float rayLength = length(rayDir);
    float3 rayDirNorm = rayDir / rayLength;

    // StartDistance 처리: StartDistance 이전에는 안개가 끼지 않음
    float excludeDistance = max(0.0f, StartDistance);
    float fogDistance = max(0.0f, rayLength - excludeDistance);

    // 안개가 없는 거리이거나 하늘인데 안개가 미치지 않는 경우 등 처리
    if (fogDistance <= 0.0f)
        return float4(0.0f, 0.0f, 0.0f, 0.0f);

    // 지수적 높이 안개 (Exponential Height Fog) 계산
    // Fog(z) = FogDensity * exp(-FogHeightFalloff * z)
    // 인테그랄을 통해 카메라부터 물체까지의 누적 안개 농도를 구함
    
    float falloff = FogHeightFalloff;
    
    // 안개가 시작되는 지점의 높이 (Camera + StartDistance * RayDir)
    float startZ = cameraPos.z + rayDirNorm.z * excludeDistance;
    float rayZ = rayDirNorm.z * fogDistance;
    
    float falloffIntegral = 1.0f;
    float exponent = falloff * rayZ;
    
    // 지수 항이 너무 작으면 선형 근사 (1 - exp(-x)) / x -> 1
    if (abs(exponent) > 0.0001f)
    {
        falloffIntegral = (1.0f - exp(-exponent)) / exponent;
    }
    
    // 시작점의 농도 계산
    float baseDensity = FogDensity * exp(-falloff * startZ);
    float fogFactor = baseDensity * fogDistance * falloffIntegral;

    // Beer-Lambert law: 1 - exp(-누적농도)
    float opacity = 1.0f - exp(-fogFactor);
    
    // Max Opacity 제한
    opacity = min(opacity, FogMaxOpacity);

    // 기존의 smoothstep을 이용한 거리 컷오프는 안개를 오히려 사라지게 만들었으므로 제거함.
    // 만약 특정 거리 이후의 안개를 강제로 자르고 싶다면 FogCutoffDistance를 다른 방식으로 활용 가능.
    // 여기서는 물체가 CutoffDistance보다 멀리 있을 경우 투명도를 그대로 유지하도록 함.
   
    return float4(FogColor.rgb, opacity);
}
