// HeightFog.hlsl
// Fullscreen post-process: Exponential Height Fog + SceneDepth visualization
// UE5 FogRendering.cpp / HeightFogCommon.ush 참고 구현

#include "Common/ConstantBuffers.hlsl"

// DepthSRV: R24_UNORM_X8_TYPELESS -> depth in [0,1]
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

// ── Helper: 하드웨어 깊이 → 월드 좌표 (CPU 계산 InvViewProj 사용) ──
float3 ReconstructWorldPos(float2 uv, float hwDepth)
{
    float4 ndc;
    ndc.x =  uv.x * 2.0 - 1.0;
    ndc.y = -(uv.y * 2.0 - 1.0);
    ndc.z = hwDepth;
    ndc.w = 1.0;

    // row-vector: worldPos = ndc * InvViewProj
    float4 worldH = mul(ndc, InvViewProj);
    return worldH.xyz / worldH.w;
}

// ── Helper: 선형 깊이 (Projection 행렬에서 추출) ──
float LinearizeDepth(float hwDepth)
{
    return Projection[3][2] / (hwDepth - Projection[2][2]);
}

// ── UE5식 Exponential Height Fog 계산 ──
// ExponentialFogParameters:
//   FogDensity * exp2(-FogHeightFalloff * (CameraZ - FogHeight))
//   = "카메라 위치에서의 밀도 (높이 감쇠 적용)"
float4 GetExponentialHeightFog(float3 worldPos)
{
    float3 camToPixel = worldPos - CameraWorldPos;
    float rayLength = length(camToPixel);

    // StartDistance 미만이면 포그 없음
    if (FogStartDistance > 0.0 && rayLength < FogStartDistance)
        return float4(0, 0, 0, 0);

    // CutoffDistance 초과이면 포그 없음
    if (FogCutoffDistance > 0.0 && rayLength > FogCutoffDistance)
        return float4(0, 0, 0, 0);

    // 카메라 높이에서의 감쇠 밀도
    float cameraRelHeight = CameraWorldPos.z - FogHeight;
    float falloff = max(FogHeightFalloff, 0.001);
    float densityAtCamera = FogDensity * exp2(-falloff * cameraRelHeight);

    // 광선 방향의 Z 성분
    float rayDirZ = camToPixel.z;

    // 광선을 따른 높이 감쇠 적분 (UE5 HeightFogCommon.ush)
    float exponent = falloff * rayDirZ;
    float lineIntegralFog;

    if (abs(exponent) > 0.01)
    {
        // 비수평: ∫ density * exp(-falloff*h) ds = densityAtCamera * (1 - exp(-exponent)) / exponent * rayLength
        lineIntegralFog = densityAtCamera * (1.0 - exp2(-exponent)) / exponent;
    }
    else
    {
        // 수평에 가까운 경우: Taylor 근사 (exponent → 0)
        lineIntegralFog = densityAtCamera;
    }

    float fogIntegral = lineIntegralFog * rayLength;

    // StartDistance 보정
    if (FogStartDistance > 0.0)
    {
        float excludeFactor = saturate((rayLength - FogStartDistance) / max(rayLength, 0.001));
        fogIntegral *= excludeFactor;
    }

    // FogFactor = 가시도 (1 = 완전 투명, 0 = 완전 안개)
    float fogFactor = saturate(exp2(-fogIntegral));

    // MaxOpacity 적용
    fogFactor = lerp(1.0 - FogMaxOpacity, 1.0, fogFactor);

    float fogOpacity = 1.0 - fogFactor;

    return float4(FogInscatteringColor.rgb * fogOpacity, fogOpacity);
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
        return float4(FogInscatteringColor.rgb * FogMaxOpacity, FogMaxOpacity);

    float3 worldPos = ReconstructWorldPos(input.uv, depth);
    float4 fogResult = GetExponentialHeightFog(worldPos);

    if (fogResult.a < 0.001)
        discard;

    return fogResult;
}
