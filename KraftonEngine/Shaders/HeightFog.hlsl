// HeightFog.hlsl
// Fullscreen post-process: Exponential Height Fog + SceneDepth visualization
// UE5 FogRendering.cpp / HeightFogCommon.ush 참고 구현
// UE5와 동일하게 exp2 (base-2) 사용

#include "Common/ConstantBuffers.hlsl"

Texture2D<float> DepthTex : register(t0);

struct PS_Input
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

//VS: Fullscreen Triangle (SV_VertexID)
PS_Input VS(uint vertexID : SV_VertexID)
{
    PS_Input output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return output;
}   

//하드웨어 깊이 → 월드 좌표 (CPU 계산 InvViewProj 사용)
float3 ReconstructWorldPos(float2 uv, float hwDepth)
{
    float4 ndc;
    ndc.x =  uv.x * 2.0 - 1.0;
    ndc.y = -(uv.y * 2.0 - 1.0);
    ndc.z = hwDepth;
    ndc.w = 1.0;

    float4 worldH = mul(ndc, InvViewProj);

    if (abs(worldH.w) < 1e-6)
        return CameraWorldPos;

    return worldH.xyz / worldH.w;
}

//선형 깊이
float LinearizeDepth(float hwDepth)
{
    return Projection[3][2] / (hwDepth - Projection[2][2]);
}

// ─── Linear Height Fog ───
// fogTop 위 = 안개 없음, fogTop~FogHeight = 선형 증가, FogHeight 아래 = 최대 밀도(1) 유지
// FogHeightFalloff = 안개 상단 페이드 범위 (1/falloff)
float4 GetLinearHeightFog(float3 worldPos)
{
    float3 camToPixel = worldPos - CameraWorldPos;
    float rayLength = length(camToPixel);

    if (rayLength < 0.001 || isnan(rayLength) || isinf(rayLength))
        return float4(0, 0, 0, 0);

    if (FogStartDistance > 0.0 && rayLength < FogStartDistance)
        return float4(0, 0, 0, 0);
    if (FogCutoffDistance > 0.0 && rayLength > FogCutoffDistance)
        return float4(0, 0, 0, 0);

    // 안개 상단 경계
    float falloff = max(FogHeightFalloff, 0.001);
    float fogTop = FogHeight + 1.0 / falloff;

    // 높이→밀도 함수: fogTop 위=0, fogTop~FogHeight=선형 0→1, FogHeight 아래=1
    // density(z) = saturate((fogTop - z) / (fogTop - FogHeight))
    float zA = CameraWorldPos.z;
    float zB = worldPos.z;
    float densityA = saturate((fogTop - zA) / (fogTop - FogHeight));
    float densityB = saturate((fogTop - zB) / (fogTop - FogHeight));

    // 둘 다 fogTop 위면 안개 없음
    if (densityA <= 0.0 && densityB <= 0.0)
        return float4(0, 0, 0, 0);

    // 광선이 fogTop을 넘는 구간은 제외
    float inFogFraction = 1.0;
    float dz = zB - zA;
    if (abs(dz) > 0.01)
    {
        float tAtTop = (fogTop - zA) / dz;
        if (dz > 0.0) // 위로 가는 광선
            inFogFraction = saturate(tAtTop);
        else           // 아래로 가는 광선
            inFogFraction = saturate(1.0 - tAtTop);

        // 카메라가 fogTop 위에 있으면 fogTop 이하 구간만
        if (zA > fogTop && zB < fogTop)
            inFogFraction = saturate((fogTop - zA) / dz) * -1.0; // 음수 dz이므 양수됨
        if (zA > fogTop && zB <= fogTop)
            inFogFraction = saturate((zA - fogTop) / (zA - zB));
        if (zA <= fogTop && zB > fogTop)
            inFogFraction = saturate((fogTop - zA) / (zB - zA));
        if (zA <= fogTop && zB <= fogTop)
            inFogFraction = 1.0;
    }
    else
    {
        // 수평 광선 — fogTop 위면 안개 없음
        if (zA > fogTop)
            return float4(0, 0, 0, 0);
    }

    // 평균 밀도 × 안개 내 이동 거리
    float avgDensity = (densityA + densityB) * 0.5;
    float fogPathLength = rayLength * inFogFraction;

    if (FogStartDistance > 0.0)
        fogPathLength = max(fogPathLength - FogStartDistance * inFogFraction, 0.0);

    float fogAmount = avgDensity * fogPathLength * FogDensity;
    float fogOpacity = saturate(fogAmount);
    fogOpacity = min(fogOpacity, FogMaxOpacity);

    return float4(FogInscatteringColor.rgb, fogOpacity);
}

// ─── Exponential Height Fog (UE5 exp2 기반) ───
//UE5식 Exponential Height Fog (exp2 기반)
// UE5 HeightFogCommon.ush와 동일한 base-2 지수 사용
float4 GetExponentialHeightFog(float3 worldPos)
{
    float3 camToPixel = worldPos - CameraWorldPos;
    float rayLength = length(camToPixel);

    if (rayLength < 0.001 || isnan(rayLength) || isinf(rayLength))
        return float4(0, 0, 0, 0);

    // StartDistance / CutoffDistance 체크
    if (FogStartDistance > 0.0 && rayLength < FogStartDistance)
        return float4(0, 0, 0, 0);
    if (FogCutoffDistance > 0.0 && rayLength > FogCutoffDistance)
        return float4(0, 0, 0, 0);

    // 언리얼 엔진의 스케일 팩터 적용
    float falloff = max(FogHeightFalloff , 0.001);
    float density = FogDensity ;

    //densityAtCamera = FogDensity * exp2(-falloff * (cameraZ - fogHeight))
    float cameraRelHeight = CameraWorldPos.z - FogHeight;
    float densityExponent = clamp(-falloff * cameraRelHeight, -127.0, 127.0);
    float densityAtCamera = density * exp2(densityExponent);

    //광선 Z 방향에 따른 높이 적분 (exp2 기반)
    float rayDirZ = camToPixel.z;
    float exponent = falloff * rayDirZ;

    float lineIntegralFog;
    if (abs(exponent) > 0.01)
    {
        float clampedExp = clamp(-exponent, -127.0, 127.0);
        lineIntegralFog = densityAtCamera * (1.0 - exp2(clampedExp)) / exponent;
    }
    else
    {
        // 수평 근사 (exponent ≈ 0)
        lineIntegralFog = densityAtCamera;
    }

    float fogIntegral = max(lineIntegralFog * rayLength, 0.0);

    // StartDistance 보정
    if (FogStartDistance > 0.0)
    {
        float excludeRatio = saturate((rayLength - FogStartDistance) / max(rayLength, 0.001));
        fogIntegral *= excludeRatio;
    }

    // UE5: 가시도 = exp(-fogIntegral)
    fogIntegral = min(fogIntegral, 20.0);
    float visibility = exp(-fogIntegral);

    // MaxOpacity 제한
    visibility = max(visibility, 1.0 - FogMaxOpacity);

    float fogOpacity = saturate(1.0 - visibility);

    return float4(FogInscatteringColor.rgb, fogOpacity);
}

float4 PS(PS_Input input) : SV_TARGET
{
    int2 coord = int2(input.position.xy);
    float depth = DepthTex.Load(int3(coord, 0)).r;

    // Sky (depth=1) — UE5 방식: 하늘은 광선 방향으로 무한 원거리 안개량 계산
    // 위를 보면 안개 옅음, 수평~아래를 보면 안개 짙음
    if (depth >= 1.0)
    {
        // NDC → 월드 방향만 추출 (위치가 아닌 방향)
        float4 ndc;
        ndc.x =  input.uv.x * 2.0 - 1.0;
        ndc.y = -(input.uv.y * 2.0 - 1.0);
        ndc.z = 0.5; // 임의 깊이
        ndc.w = 1.0;
        float4 worldH = mul(ndc, InvViewProj);
        float3 worldDir = normalize(worldH.xyz / worldH.w - CameraWorldPos);

        // 가상 거리를 매우 멀리 잡아서 안개 계산
        float skyDist = 100000.0;
        float3 skyWorldPos = CameraWorldPos + worldDir * skyDist;
        float4 fogResult = bUseLinearFog ? GetLinearHeightFog(skyWorldPos) : GetExponentialHeightFog(skyWorldPos);

        if (any(isnan(fogResult)) || any(isinf(fogResult)))
            discard;
        if (fogResult.a < 0.001)
            discard;
        return fogResult;
    }

    float3 worldPos = ReconstructWorldPos(input.uv, depth);
    float4 fogResult = bUseLinearFog ? GetLinearHeightFog(worldPos) : GetExponentialHeightFog(worldPos);

    // NaN/Inf 최종 방어
    if (any(isnan(fogResult)) || any(isinf(fogResult)))
        discard;

    if (fogResult.a < 0.001)
        discard;

    return fogResult;
}
