// HeightFog.hlsl
// Fullscreen post-process: Exponential Height Fog + SceneDepth visualization
// UE5 FogRendering.cpp / HeightFogCommon.ush 참고 구현

#include "Common/ConstantBuffers.hlsl"

Texture2D<float> DepthTex : register(t0);

struct PS_Input
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// ── VS: Fullscreen Triangle (SV_VertexID) ──
PS_Input VS(uint vertexID : SV_VertexID)
{
    PS_Input output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return output;
}

// ── 하드웨어 깊이 → 월드 좌표 (CPU 계산 InvViewProj 사용) ──
float3 ReconstructWorldPos(float2 uv, float hwDepth)
{
    float4 ndc;
    ndc.x =  uv.x * 2.0 - 1.0;
    ndc.y = -(uv.y * 2.0 - 1.0);
    ndc.z = hwDepth;
    ndc.w = 1.0;

    // row-vector 규약: worldPos = ndc * InvViewProj
    float4 worldH = mul(ndc, InvViewProj);

    // NaN 방지: w가 0에 근접하면 카메라 위치 반환 (포그 없음)
    if (abs(worldH.w) < 1e-6)
        return CameraWorldPos;

    return worldH.xyz / worldH.w;
}

// ── 선형 깊이 ──
float LinearizeDepth(float hwDepth)
{
    return Projection[3][2] / (hwDepth - Projection[2][2]);
}

// ── UE5식 Exponential Height Fog ──
// 결과: float4(fogColor, fogOpacity)
//   fogOpacity=0 → 안개 없음,  fogOpacity=1 → 완전 안개
float4 GetExponentialHeightFog(float3 worldPos)
{
    float3 camToPixel = worldPos - CameraWorldPos;
    float rayLength = length(camToPixel);

    // 비정상 거리 방지
    if (rayLength < 0.001 || isnan(rayLength) || isinf(rayLength))
        return float4(0, 0, 0, 0);

    // StartDistance / CutoffDistance 체크
    if (FogStartDistance > 0.0 && rayLength < FogStartDistance)
        return float4(0, 0, 0, 0);
    if (FogCutoffDistance > 0.0 && rayLength > FogCutoffDistance)
        return float4(0, 0, 0, 0);

    float falloff = max(FogHeightFalloff, 0.001);

    // 카메라 높이 기준 밀도
    float cameraRelHeight = CameraWorldPos.z - FogHeight;
    // exp 오버플로 방지: 지수 범위 제한
    float densityExponent = clamp(-falloff * cameraRelHeight, -80.0, 80.0);
    float densityAtCamera = FogDensity * exp(densityExponent);

    // 광선 Z 성분에 따른 높이 적분
    float rayDirZ = camToPixel.z;
    float exponent = falloff * rayDirZ;

    float lineIntegralFog;
    if (abs(exponent) > 0.01)
    {
        // 비수평 광선: 해석적 적분 (exp 오버플로 방지)
        float clampedExp = clamp(-exponent, -80.0, 80.0);
        lineIntegralFog = densityAtCamera * (1.0 - exp(clampedExp)) / exponent;
    }
    else
    {
        // 수평 근사
        lineIntegralFog = densityAtCamera;
    }

    // 음수 적분 방지
    float fogIntegral = max(lineIntegralFog * rayLength, 0.0);

    // StartDistance 보정
    if (FogStartDistance > 0.0)
    {
        float excludeRatio = saturate((rayLength - FogStartDistance) / max(rayLength, 0.001));
        fogIntegral *= excludeRatio;
    }

    // 가시도 계산 (적분값 범위 제한)
    fogIntegral = min(fogIntegral, 20.0);
    float visibility = exp(-fogIntegral);

    // MaxOpacity 제한: visibility 최저값 = 1 - MaxOpacity
    visibility = max(visibility, 1.0 - FogMaxOpacity);

    float fogOpacity = saturate(1.0 - visibility);

    //    블렌드: SrcAlpha * Src.rgb + (1-SrcAlpha) * Dst.rgb
    //    → fogOpacity * fogColor + visibility * sceneColor  (정확한 포그 합성)
    return float4(FogInscatteringColor.rgb, fogOpacity);
}

// ── PS ──
float4 PS(PS_Input input) : SV_TARGET
{
    int2 coord = int2(input.position.xy);
    float depth = DepthTex.Load(int3(coord, 0)).r;

    // ═══════════ SceneDepth 시각화 ═══════════
    if (bSceneDepthMode)
    {
        if (depth >= 1.0)
            return float4(0, 0, 0, 1);

        float linearZ = LinearizeDepth(depth);
        float nearZ = FogNearPlane;
        float farZ = FogFarPlane;
        if (farZ <= nearZ) farZ = nearZ + 1000.0;

        float normalized = saturate((linearZ - nearZ) / (farZ - nearZ));
        float gray = 1.0 - normalized;
        return float4(gray, gray, gray, 1);
    }

    // ═══════════ Exponential Height Fog ═══════════

    // Sky (depth=1) → 완전 안개
    if (depth >= 1.0)
        return float4(FogInscatteringColor.rgb, FogMaxOpacity);

    float3 worldPos = ReconstructWorldPos(input.uv, depth);
    float4 fogResult = GetExponentialHeightFog(worldPos);

    // NaN/Inf 최종 방어: 결과가 비정상이면 그리지 않음
    if (any(isnan(fogResult)) || any(isinf(fogResult)))
        discard;

    if (fogResult.a < 0.001)
        discard;

    return fogResult;
}
